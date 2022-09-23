// We use 17.14 float form

#define F (1 << 14) // F = 1.0
#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-(1 << 31))

int intToFp (int n);
int fpToInt_round (int x);
int fpToInt_abandon (int x);
int add_fp (int x, int y);
int add_fp_int (int x, int n);
int sub_fp (int x, int y);
int sub_fp_int (int x, int n);
int mul_fp (int x, int y);
int mul_fp_int (int x, int n);
int div_fp (int x, int y);
int div_fp_int (int x, int n);

int intToFp (int n)
{
    return n * F;
}
int fpToInt_round (int x)
{
    if (x >= 0) return (x + F / 2) / F;
    else return (x - F/2) / F;
}
int fpToInt_abandon (int x)
{
    return x / F;
}
int add_fp (int x, int y)
{
    return x + y;
}
int add_fp_int (int x, int n)
{
    return x + n * F;
}
int sub_fp (int x, int y)
{
    return x - y;
}
int sub_fp_int (int x, int n)
{
    return x - n * F;
}
int mul_fp (int x, int y)
{
    return ((int64_t) x) * y / F;
}
int mul_fp_int (int x, int n)
{
    return x * n;
}
int div_fp (int x, int y)
{
    return ((int64_t) x) * F / y;
}
int div_fp_int (int x, int n)
{
    return x / n;
}