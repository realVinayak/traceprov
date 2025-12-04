#include "c.h"
#include "postgres.h"
#include "access/hash.h"
#include "traceprov.h"


// Copied from postgres.
// It is possible to call the corresponding postgres function (for hashing int64)
// But, it'll then have to be a direct call (calling the corresponding the function within postgres)
// Using this, we don't have that overhead.
uint32 traceprov_hashint8(int64 value)
{
    /*
     * The idea here is to produce a hash value compatible with the values
     * produced by hashint4 and hashint2 for logically equal inputs; this is
     * necessary to support cross-type hash joins across these input types.
     * Since all three types are signed, we can xor the high half of the int8
     * value if the sign is positive, or the complement of the high half when
     * the sign is negative.
     */
    int64   val = value;
    uint32  lohalf = (uint32) val;
    uint32  hihalf = (uint32) (val >> 32);

    lohalf ^= (val >= 0) ? hihalf : ~hihalf;

    return hash_uint32(lohalf);
}