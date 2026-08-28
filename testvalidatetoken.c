#include "valkey.h"
#include <string.h>

/* Add user and set password automatically */
int add_user_and_set_password_on_load(VKModuleCtx *ctx) {
    // Define the username and password
    const char *username = "default_user";
    const char *password = "secure_password";

    // Check if the user already exists
    if (VK_UserExists(ctx, username)) {
        // If the user exists, update the password
        if (VK_SetUserPassword(ctx, username, password) != VK_OK) {
            VK_Log(ctx, "ERROR: Failed to update password for existing user '%s'", username);
            return VK_ERR;
        }
        VK_Log(ctx, "INFO: Password updated for existing user '%s'", username);
    } else {
        // If the user does not exist, add the user and set the password
        if (VK_AddUser(ctx, username, password) != VK_OK) {
            VK_Log(ctx, "ERROR: Failed to add user '%s'", username);
            return VK_ERR;
        }
        VK_Log(ctx, "INFO: User '%s' added with password", username);
    }

    return VK_OK;
}

/* Module initialization function */
int VK_MODULE_INIT_FUNC(VKModuleCtx *ctx) {
    // Automatically add a user and set their password when the module is loaded
    if (add_user_and_set_password_on_load(ctx) != VK_OK) {
        return VK_ERR;
    }

    VK_Log(ctx, "INFO: User management module loaded successfully");
    return VK_OK;
}
