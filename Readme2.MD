We kept auth and audit in the same module because audit needs the same context produced during authentication. This avoids duplicate parsing, reduces latency, and ensures every allow/deny decision has a consistent audit trail.


Valkey does not have a high-level module bus, but module-to-module communication is possible through ValkeyModule_Call, shared keys or Streams, and lower-level shared APIs. For auth and audit, I would be careful about synchronous module-to-module calls because auth is in the command hot path. A safer design is for the auth logic to emit audit events asynchronously, for example into a Stream or background buffer, so authentication latency is not impacted.

There is also a security concern. Audit depends on the auth context, such as identity, token claims, command, resource, and allow/deny decision. If that context is passed through a public command or exposed interface, audit events could be spoofed, skipped, or modified. So either auth and audit should stay in the same module, or the communication should use an internal/private interface with clear failure handling.

Cons of module-to-module communication
It adds latency in the hot path, especially if every command triggers another module call.
It adds coupling between modules: version mismatch or API change can break behavior.
It increases failure handling complexity: what happens if auth succeeds but audit fails?
It makes debugging harder because one command execution may pass through multiple modules.
It may affect Valkey stability if communication is synchronous, blocking, or memory-heavy.


#include "valkeymodule.h"
#include <string.h>

/* Optional: redact secrets; do not log passwords/tokens. */

static int auth_cb(ValkeyModuleCtx *ctx,
                   ValkeyModuleString *username,
                   ValkeyModuleString *password,
                   ValkeyModuleString **err)
{
    const char *user = ValkeyModule_StringPtrLen(username, NULL);
    const char *pwd  = ValkeyModule_StringPtrLen(password, NULL);

    /* Example validation logic (replace with your real check) */
    if (!strcmp(user, "foo") && !strcmp(pwd, "valid_password")) {
        /* Mark the connection as authenticated as ACL user "foo" */
        ValkeyModule_AuthenticateClientWithACLUser(ctx, "foo", 3, NULL, NULL, NULL);
        ValkeyModule_Log(ctx, "notice", "AUTH SUCCESS user=%s", user);
        return VALKEYMODULE_AUTH_HANDLED;
    }

    /* Deny */
    ValkeyModule_Log(ctx, "notice", "AUTH FAIL user=%s", user);

    const char *msg = "WRONGPASS invalid username-password pair";
    *err = ValkeyModule_CreateString(ctx, msg, strlen(msg));
    return VALKEYMODULE_AUTH_HANDLED;
}

int ValkeyModule_OnLoad(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    VALKEYMODULE_NOT_USED(argv);
    VALKEYMODULE_NOT_USED(argc);

    if (ValkeyModule_Init(ctx, "authfilter", 1, VALKEYMODULE_APIVER_1) == VALKEYMODULE_ERR)
        return VALKEYMODULE_ERR;

    /* Register module-based authentication callback */
    ValkeyModule_RegisterAuthCallback(ctx, auth_cb);  /* shown in Valkey docs :contentReference[oaicite:2]{index=2} */

    ValkeyModule_Log(ctx, "notice", "authfilter loaded (module auth callback active)");
    return VALKEYMODULE_OK;
}
