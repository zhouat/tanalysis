int main(int argc, char** argv)
{
    int x = foo (10);
    int y = foo (foo_iterative(argc));
    int tainted;
    int z = foo (tainted);
    int t = foo_iterative(z);
    int q = foo (foo_iterative(tainted));
    
    return 0;
}

int foo_iterative(int n)
{
    int prev;
    int prev_prev;
    int i;
    int aux;
    
    prev = 1;
    prev_prev = 1;
    
    if (n < 1)
        return 0;
    for (i = 2; i <= n; ++ i) {
        aux = prev + prev_prev;
        prev = prev_prev;
        prev_prev = aux;
    }
    return prev_prev;
}

int foo(int n) 
{
    int r;
    if (n > 1)
        r = n * bar(n);
    else 
        r = 1;
    return r;
}

int bar(int n)
{
    int r;
    if (n > 1)
        r = n * foo(n);
    else 
        r = 1;
    return r;
} 