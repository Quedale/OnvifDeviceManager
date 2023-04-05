#ifndef CRED_INPUT_H_ 
#define CRED_INPUT_H_

#include <gtk/gtk.h>

typedef struct {
    int visible;
    GtkWidget * root;
    void (*login_callback)();
    void (*cancel_callback)();
    void * user_data;
    void * private;
} CredentialsDialog;

typedef struct { 
    char * user;
    char * pass;
    CredentialsDialog * dialog;
    void * user_data;
} LoginEvent;

CredentialsDialog * CredentialsDialog__create(void (*login_callback)(LoginEvent *), void (*cancel_callback)(CredentialsDialog *));
void CredentialsDialog__destroy(CredentialsDialog* dialog);
void CredentialsDialog__show(CredentialsDialog* dialog, void * user_data);
void CredentialsDialog__hide(CredentialsDialog* dialog);
LoginEvent * LoginEvent_copy(LoginEvent * original);

#endif