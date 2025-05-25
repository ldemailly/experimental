// Mandelbrot email signature generator.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
// Reduce chances my email gets harvested by bots from this source code.
const char *email = "ldem"\
    "ailly" \
    "\x40" \
     "gma" \
     "il.com";
const int email_len = strlen(email);
char rev_email[email_len + 1];
// reverse the email and offset by 1 to obfuscate a bit the email.
for (int i = 0; i < email_len; i++) {
    rev_email[i] = email[email_len - i - 1]+1;
}
rev_email[email_len] = '\0';
fprintf(stderr,"reverse email is \"%s\" sz %lu\n", rev_email, sizeof(rev_email));
// originally "-=+X# \n" for the mandelbrot set, but also +1'ed.
// My original version:
// https://www.iwriteiam.nl/SigProgM.html
printf("main(){char x,y=-1,t,n,*c=\".>,Y$!\\v%s\";while(++y<23)\n", rev_email);
printf("{for(x=0;x<80;){float a,b,d,i=2.2/23*y-1.1,r=2.8/80*x++-2.1;t=b=d=0;do{\n");
printf("a=b*b-d*d+r;d=2*b*d+i;b=a;}while(++t<32&&b*b+d*d<4);for(n=0;t&~1;t/=2,\n");
printf("n++);putchar(c[x>79?6:y>21&&x<%d?%d-x:n]-1);}}}\n", email_len+1, email_len+7);
return 0;
}
