//----------------------------------------------
//
// Integer square root. Take the square root of an integer.
//

#define step(shift) \
    if((0x40000000l >> shift) + root <= value)          \
    {                                                   \
        value -= (0x40000000l >> shift) + root;         \
        root = (root >> 1) | (0x40000000l >> shift);    \
    }                                                   \
    else                                                \
    {                                                   \
        root = root >> 1;                               \
    }

int
iSqrt(int value)
{
  int root = 0;

  step( 0);
  step( 2);
  step( 4);
  step( 6);
  step( 8);
  step(10);
  step(12);
  step(14);
  step(16);
  step(18);
  step(20);
  step(22);
  step(24);
  step(26);
  step(28);
  step(30);

  // round to the nearest integer, cuts max error in half

  if(root < value) {
    ++root;
  }

  return root;
}



