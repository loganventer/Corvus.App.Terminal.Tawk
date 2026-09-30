#ifndef APP_CORE_PROFILE_FIELD_H
#define APP_CORE_PROFILE_FIELD_H

/* A part of your own profile that can be changed. */
typedef enum ProfileField {
    PROFILE_FIELD_NAME = 0,
    PROFILE_FIELD_ABOUT,
    PROFILE_FIELD_PICTURE,
    PROFILE_FIELD_COUNT
} ProfileField;

/* The protocol name: name, about or picture. */
const char  *profile_field_name(ProfileField field);
ProfileField profile_field_parse(const char *name);   /* PROFILE_FIELD_COUNT when unknown */
/* For people: Name, About or Photo. */
const char  *profile_field_label(ProfileField field);

#endif
