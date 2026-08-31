#include "tagged_union.hpp"

TAGGED_UNION(TypeData, None,
    (None, struct { }),
    (Any,  struct { }),
    (Bang, struct { }),
    (Unit, struct { }),
    (Macro, struct {int a;}),
    (Primitive, struct {int a;int b;})
);