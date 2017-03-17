#ifndef LINKAGE_H
#define LINKAGE_H

#ifdef __ASSEMBLY__

#define GLOBAL(name)    \
        .globl name;    \
        name:

#define END(name) \
        .size name, .-name

#define ENDPROC(name) \
        .type name, @function; \
        END(name)

#endif /* __ASSEMBLY__ */

#endif /* LINKEGE_H */
