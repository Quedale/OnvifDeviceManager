#ifndef CRED_INPUT_H_ 
#define CRED_INPUT_H_

#include <gtk/gtk.h>
#include "dialog/app_dialog.h"

typedef struct _CredentialsDialog CredentialsDialog;

typedef struct _CredentialsDialog {
    AppDialog parent;
    void * elements;
} CredentialsDialog;

CredentialsDialog * CredentialsDialog__create();
const char * CredentialsDialog__get_username(CredentialsDialog * self);
const char * CredentialsDialog__get_password(CredentialsDialog * self);

#endif