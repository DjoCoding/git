#ifndef ARGS_H_
#define ARGS_H_

typedef struct {
    char **values;
    int count;
} Args;

#define Self Args

Self args_init(int argc, char *argv[]);
bool args_end(Self self);
char *args_peek(Self self);
char *args_consume(Self *self);


#ifdef ARGS_IMPLEMENTATION_

#include <assert.h>

Self args_init(int argc, char *argv[]) {
    Self args = {
        .count = argc,
        .values = argv
    };
    return args;
}


bool args_end(Self self) {
    return self.count == 0;
}

char *args_peek(Self self) {
    assert(!args_end(self));
    return self.values[0];
}

char *args_consume(Self *self) {
    assert(!args_end(*self));
    
    char *arg = self->values[0];

    self->count -= 1;
    self->values += 1;

    return arg;
}


#endif // ARGS_IMPLEMENTATION_

#undef Self

#endif // ARGS_H_