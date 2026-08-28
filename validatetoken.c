#include "valkeymodule.h"
#include <string.h>

/* Validate the token (custom logic) */
int isTokenValid(const char *token) {
    // Example validation: Replace with actual token verification logic
    return strcmp(token, "valid_token") == 0;
}

/* Authenticate the client dynamically with a module-defined user */
int authenticateWithModuleUser(ValkeyModuleCtx *ctx) {
    // Static user to ensure it persists across function calls
    static ValkeyModuleUser *moduleUser = NULL;

    // Create the module user if it doesn't exist
    if (!moduleUser) {
        moduleUser = ValkeyModule_CreateModuleUser("module_user");
        ValkeyModule_SetModuleUserACL(moduleUser, "allcommands"); // Allow all commands
        ValkeyModule_SetModuleUserACL(moduleUser, "allkeys");     // Allow access to all keys
        ValkeyModule_SetModuleUserACL(moduleUser, "on");          // Activate the user
    }

    // Authenticate the client using the module user
    ValkeyModule_AuthenticateClientWithUser(ctx, moduleUser, NULL, NULL, NULL);

    return 1; // Successful authentication
}

/* Command to validate the token and execute the command */
int ValidateTokenAndExecuteCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        return ValkeyModule_WrongArity(ctx); // Requires at least token and command
    }

    // Extract the token (first argument)
    ValkeyModuleString *tokenArg = argv[1];
    const char *token = ValkeyModule_StringPtrLen(tokenArg, NULL);

    // Validate the token
    if (!isTokenValid(token)) {
        ValkeyModule_ReplyWithError(ctx, "ERR Invalid token");
        return VALKEYMODULE_OK;
    }

    // Authenticate the client dynamically with the module-defined user
    if (!authenticateWithModuleUser(ctx)) {
        ValkeyModule_ReplyWithError(ctx, "ERR Module user authentication failed");
        return VALKEYMODULE_OK;
    }

    // Execute the remaining command (all arguments after the token)
    ValkeyModuleCallReply *reply = ValkeyModule_Call(ctx, "COMMAND", "v", argv + 2, argc - 2);
    if (!reply) {
        ValkeyModule_ReplyWithError(ctx, "ERR Command execution failed");
        return VALKEYMODULE_OK;
    }

    // Return the result of the command to the client
    ValkeyModule_ReplyWithCallReply(ctx, reply);
    ValkeyModule_FreeCallReply(reply);

    return VALKEYMODULE_OK;
}

/* OnLoad: Register the custom command */
int ValkeyModule_OnLoad(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (ValkeyModule_Init(ctx, "authmodule", 1, VALKEYMODULE_APIVER_1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    // Register the command "authmodule.validatetoken"
    if (ValkeyModule_CreateCommand(ctx, "authmodule.validatetoken", ValidateTokenAndExecuteCommand, "write", 0, 0, 0) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
}
