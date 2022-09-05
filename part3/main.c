#define WHITE_BG 0xf0

void main() {
    __asm__("mov $0xb800, %ax");
    __asm__("mov %ax, %ds");
    char s[] = {'h','e','l','l','o',',',' ','w','o','r','l','d'};
    char *vga_con = (char *)0;
    *vga_con = s[0];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[1];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[2];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[3];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[4];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[5];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[6];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[7];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[8];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[9];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[10];
    vga_con++;
    *vga_con = WHITE_BG;
    vga_con++;

    *vga_con = s[11];
    vga_con++;
    *vga_con = WHITE_BG;

    while (1);
}