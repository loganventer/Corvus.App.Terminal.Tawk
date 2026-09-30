#ifndef APP_CLIENTS_CLI_DOCTOR_H
#define APP_CLIENTS_CLI_DOCTOR_H

#include "clients/cli/doctor_inputs.h"

/* Prints a checklist of what tawk needs at runtime (terminal, folders,
 * backend, audio, media viewer, screensaver) with a hint for anything
 * missing. Returns 0 when nothing required is missing, 1 otherwise. */
int doctor_run(const DoctorInputs *in);

#endif
