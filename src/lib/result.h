#ifndef RESULT_H_
#define RESULT_H_

typedef struct {
    bool ok;
    union {
        const char *error;
        void *data;
    } as;
} Result;

Result result_ok(void *data);
Result result_error(const char *error);

#ifdef RESULT_IMPLEMENTATION_

Result result_ok(void *data) {
    return (Result) {
        .ok = true,
        .as.data = data
    };
}

Result result_error(const char *error) {
    return (Result) {
        .ok = false,
        .as.error = error
    };
}

#endif // RESULT_IMPLEMENTATION_

#endif // _RESULT_H