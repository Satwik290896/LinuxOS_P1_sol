void main() {
    __asm__("mov $0xb800, %ax");
    __asm__("mov %ax, %ds");
    int i;
    char s[] = {'h','e','l','l','o',',',' ','w','o','r','l','d'};
    char *vga_con = (char *)0;
    for (i = 0; i < 12; ++i) {
        __asm__("mov $0, %ax");
        __asm__("mov %ax, %ds");
        char c = s[i];
        __asm__("mov $0xb800, %ax");
        __asm__("mov %ax, %ds");
        *vga_con = c;
        vga_con++;
        *vga_con = (char)0xf0;
        vga_con++;
    }
    while (1);
}