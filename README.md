# float_reduction

This is about testing floating point numbers to see if they are whole integers and can be converted to an int32_t without loss.
It is part of the dCBOR standards discussion.

https://datatracker.ietf.org/doc/draft-mcnally-deterministic-cbor/

TLDR: I’ve come to understand that all whole integers are precisely represented in IEEE 754. No rounding is needed to convert to CBOR type 0/1.

I also figured out a simple way to check if a float is a whole integer with only shifts and masks and a little loop by looking inside the IEEE 754 representation.


Details:

I knew that you couldn't represent some numbers like 0.10 exactly as a float, only something like 0.10000000149. I didn’t know whether all whole integers could be represented exactly. If they couldn’t be represented exactly, rounding would be required and that might not be done exactly the same everywhere. Numeric reduction would be not exactly the same everywhere.

Then I read about bugs in Intel chips that manifest in ceilf(), a candidate function for the whole integer check.
https://randomascii.wordpress.com/2014/01/27/theres-only-four-billion-floatsso-test-them-all/

I had to look deeper for the confidence that the CPU float instructions would give *exactly* the same result on hundreds of CPUs from Intel, Atmel, Apple, Infineon…


Here’s an interesting result --- all you have to do is check that the number of used bits in the significand are less than the exponent!

For example take the representation of 3:
- The exponent is 1 which means the multiply by 2 (shift by 1)
- The significand uses one bit for a value of 1/2  (plus 1 is added)
- 1 1/2 multiplied by 2 is 3 

Or 9:
- The exponent is 3 which means multiply by 8 (shift by 3)
- The significand uses three bits  to just be 1/8 (the bits for 1/2 and 1/4 are 0; plus 1 is always added)
- 1 1/8 multiplied by 8 is 9

This is a nice explanation of the float format.
https://softwareengineering.stackexchange.com/questions/215065/can-anyone-explain-representation-of-float-in-memory

You can implement this without using floating-point HW or libraries — convert the float bits to a uint32_t and do some shifts, masks and a little loop.


Here’s 6 ways to check that a float is a whole integer:
   - ceilf(f) == f
   - floorf(f) == f
   - rint(f) == f
   - intnearby(f) == f
   - cast to int and back to float and see the same value (the compiler uses floating-point HW to implement this)
   - my method of checking significand bits used relative to the exponent

Note that probably the same floating-point HW and SW underlies all the languages on a given platform. You get the same implementation working in C, Rust, Swift, Java, Python...

I’ve implemented all of these and confirmed they give exactly the same result for all single precision float values (there’s only 2^32 of them) on Intel with MacOS. You can get the implementation here and run it on your platform if you want.

Testing all the double values takes too long.

Ceilf() and floor() compile down to one inline instruction on Intel and are probably the least code.

The cast method only works for floats less that UINT32_MAX. This is OK for CBOR numeric reduction, but it is not a universal solution like all the others.

Additionally, here’s a good articple on determinism in floating-point.
https://randomascii.wordpress.com/2013/07/16/floating-point-determinism/

LL


