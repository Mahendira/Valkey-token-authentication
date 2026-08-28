#include "valkeymodule.h"
#include <string.h>

/* Validate the token (custom logic) */
int isTokenValid(const char *token) {
    // Replace with your actual token validation logic
    return strcmp(token, "valid_token") == 0;
}

/* Create a module user with the required permissions */
ValkeyModuleUser *getOrCreateModuleUser() {
    static ValkeyModuleUser *moduleUser = NULL;

    if (!moduleUser) {
        // Create a module user if it doesn't exist
        moduleUser = ValkeyModule_CreateModuleUser("module_user");
        ValkeyModule_SetModuleUserACL(moduleUser, "allcommands"); // Allow all commands
        ValkeyModule_SetModuleUserACL(moduleUser, "allkeys");     // Allow access to all keys
        ValkeyModule_SetModuleUserACL(moduleUser, "on");          // Activate the user
    }

    return moduleUser;
}

/* Command Filter: Intercepts all commands, validates token, and uses module user */
int CommandFilter(ValkeyModuleCommandFilterCtx *filterCtx) {
    // Get the command name
    ValkeyModuleString *cmdArg = ValkeyModule_CommandFilterCtxGetArg(filterCtx, 0);
    const char *cmd = ValkeyModule_StringPtrLen(cmdArg, NULL);

    // Check if the command is AUTH
    if (strcasecmp(cmd, "AUTH") == 0) {
        // Ensure the AUTH command has the required token argument
        int argc = ValkeyModule_CommandFilterCtxGetArgCount(filterCtx);
        if (argc < 2) {
            ValkeyModule_CommandFilterCtxSetError(filterCtx, "ERR AUTH command requires a token");
            return VALKEYMODULE_OK;
        }

        // Extract the token (second argument in AUTH command)
        ValkeyModuleString *tokenArg = ValkeyModule_CommandFilterCtxGetArg(filterCtx, 1);
        const char *token = ValkeyModule_StringPtrLen(tokenArg, NULL);

        // Validate the token
        if (!isTokenValid(token)) {
            ValkeyModule_CommandFilterCtxSetError(filterCtx, "ERR Invalid token in AUTH command");
            return VALKEYMODULE_OK;
        }

        // Authenticate the client with the module user
        ValkeyModuleUser *moduleUser = getOrCreateModuleUser();
        ValkeyModule_AuthenticateClientWithUser(
            ValkeyModule_CommandFilterCtxGetClientCtx(filterCtx), moduleUser, NULL, NULL, NULL);

        // Token is valid, allow the AUTH command to proceed
        return VALKEYMODULE_OK;
    }

    // Validate token for non-AUTH commands
    int argc = ValkeyModule_CommandFilterCtxGetArgCount(filterCtx);
    if (argc < 1) {
        ValkeyModule_CommandFilterCtxSetError(filterCtx, "ERR Missing token");
        return VALKEYMODULE_OK;
    }

    // Assume the token is the first argument of non-AUTH commands
    ValkeyModuleString *tokenArg = ValkeyModule_CommandFilterCtxGetArg(filterCtx, 0);
    const char *token = ValkeyModule_StringPtrLen(tokenArg, NULL);

    // Validate the token
    if (!isTokenValid(token)) {
        ValkeyModule_CommandFilterCtxSetError(filterCtx, "ERR Invalid token");
        return VALKEYMODULE_OK;
    }

    // Authenticate the client with the module user
    ValkeyModuleUser *moduleUser = getOrCreateModuleUser();
    ValkeyModule_AuthenticateClientWithUser(
        ValkeyModule_CommandFilterCtxGetClientCtx(filterCtx), moduleUser, NULL, NULL, NULL);

    // Token is valid, allow the command to proceed
    return VALKEYMODULE_OK;
}

/* OnLoad: Register the command filter */
int ValkeyModule_OnLoad(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    // Initialize the module
    if (ValkeyModule_Init(ctx, "authmodule", 1, VALKEYMODULE_APIVER_1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    // Register the global command filter
    if (ValkeyModule_RegisterCommandFilter(ctx, CommandFilter, VALKEYMODULE_CMDFILTER_NO_INJECT) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
}

/* OnUnload: Cleanup logic */
int ValkeyModule_OnUnload(ValkeyModuleCtx *ctx) {
    VALKEYMODULE_NOT_USED(ctx);
    return VALKEYMODULE_OK;
}
