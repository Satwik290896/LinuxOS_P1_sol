#define FB_MAX_COL 80
#define FB_MAX_ROW 24
#define WHITE_BG_BLACK_FG 0xf0
#define STR_LEN 12

void main() {
    __asm__("mov $0xb800, %ax");
    __asm__("mov %ax, %ds");
    char s[] = {'h','e','l','l','o',',',' ','w','o','r','l','d'};
    char *center_of_con = (char *)((FB_MAX_ROW / 2 * FB_MAX_COL + \
                                    (FB_MAX_COL / 2) - (STR_LEN / 2)) * 2);
    *center_of_con = s[0];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[1];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[2];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[3];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[4];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[5];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[6];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[7];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[8];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[9];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[10];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;
    center_of_con++;

    *center_of_con = s[11];
    center_of_con++;
    *center_of_con = WHITE_BG_BLACK_FG;

    while (1);
}