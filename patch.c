
extern void rep_symbol(int i);
extern void target_func(int i);

void rep_symbol(int i)
{
    printk("rep_symbol: PATCHED.... %d\n", i);

    return;
}

void target_func(int i)
{
    printk("target_func(): ANYTHING!.. %d\n", i);

    return;
}
