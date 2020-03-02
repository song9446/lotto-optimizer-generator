/* Source file: verify.c */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define VERSION "1.20"
/* verify 1.20 (C) Richard K. Lloyd and
                   Connect Internet Solutions Limited 1996-2002.

   This program verifies a ticket wheel for a guaranteed 2-match or 3-match.

   New features with 1.20:
   Ability to restrict the number of losing combinations
   reported (-l option).
   Increased upper limit of balls to 54.
   You can now specify when to stop progress reports (-c option).
   Large numbers are now formatted with commas.

   Syntax: verify [-b <balls available>] [-c <report cutoff>] [-h]
                  [-l <losing combs limit>] [-m <matches>]
                  [-t <numbers per ticket>] [-?] [-h] [filename]

   If no options are supplied, the default is "-b 49 -c 30 -l 10 -m 3 -t 6",
   which therefore checks a 6 from 49 lottery for a guaranteed 3-match, only
   listing the first 10 losing combinations (if there are any, which hopefully
   isn't the case !) and switching off progress reports (including losing
   combinations) when there is 30 seconds or less to go.

   If the filename is omitted, then standard input is used.
   The file should be a list of ticket numbers (one ticket per line):
   e.g.
    1  2  3  4  5  6
   27 32 33 37 42 48
   and so on.

   Blank lines are ignored and you can use a hash (#) in the first column for
   comments. Although the numbers for each ticket don't have to be in ascending
   order (in a particular line), you will get an ignorable warning if they
   aren't (I sort them after reading in anyway).

   IMPORTANT: Compile this program with as much optimisation as you can !
   For HP-UX: cc +O3 -Ae +ESlit +w1 -s -o verify.bin verify.c
   For gcc:   gcc -ansi -Wall -O4 -funroll-loops -s -o verify.bin verify.c
   Typical execution time (with full optimisation) for a guaranteed 3-match
   check 6 from 49 lottery: 30 seconds on an HP 9000 712/60 (roughly
   equivalent to a high-end Pentium).

   PC USERS: You may require GNU getopt.c and getopt.h as well when building
             this !
*/

/* Number of balls in machine. UK lottery = 49. Can be specified with the
   -b option. I've never seen a lottery with more than 54 balls. Let me know
   if you have ! */
#define DEF_UPPER 45   /* Default number of balls in machine (UK lottery) */
#define MAX_UPPER 54 /* You cannot specify more than this number of balls */

/* At least one guaranteed MIN_MATCH checked for. Change with -m option,
   but the code currently only copes with -m 2 or -m 3 */
#define MIN_MATCH 3    /* Lowest number of matches in UK lottery for a prize */

/* Numbers required per ticket. UK lottery = 6. Can be specified with the
   -t option. */
#define DEF_DRAWN 6    /* Default numbers per ticket (UK lottery) */
#define MAX_DRAWN 7    /* Can't go higher than this */

/* Maximum number of combinations of a 3 from MAX_DRAWN ticket =
   (MAX_DRAWN!/(3!*(MAX_DRAWN-3)!) = 7!/(6*4!) = 35.
   Note that if you change MAX_DRAWN above, you must change MAX_COMBS too */
#define MAX_COMBS 35

#define DEF_CUTOFF 30 /* Cut off reporting with 30 seconds to go */
#define MAX_CUTOFF 10*60 /* Maximum cutoff point (10 minutes) */
#define DEF_LOSING_REP 10 /* Default number of losing combinations reported */

char combs[MAX_UPPER][MAX_UPPER+1][MAX_UPPER+2]; /* Char array saves memory */
int balls[MAX_DRAWN+1],pt[MAX_DRAWN+1];
int perms[MAX_COMBS+1][4];
long tot_combs;
int freq,done_rep=0;
long tcount=0;
char buff[512];
time_t st_time;
FILE *inhand;
double fact[MAX_UPPER+1]; /* Reaches MAX_UPPER!, so need an FP array */
int min_match=MIN_MATCH,drawn=DEF_DRAWN,upper=DEF_UPPER,num_combs;
int cutoff=DEF_CUTOFF;

long missed=0,covered=0,losing_rep=DEF_LOSING_REP;
long cover[256];
int l=256,bs=-1;

#ifndef __hpux
extern int optind;
extern char *optarg;
#endif

long calc_combs(numsel,numset)
/* Calculation numsel from numset combinations */
int numsel,numset;
{
   return((long)(fact[numset]/(fact[numsel]*fact[numset-numsel])+0.5));
}

char *mills(num)
long num;
{
   /* This routine is a stripped-down version of the one I use to display
      large numbers on my lottery WWW pages. It's not quite as flexible as
      my WWW version, but it does the task required of it.
      Oh, it returns an comma-filled string of the long int passed to it */

   static char mls[32];
   long mil=num/1000000;
   int ln,p=0;
   (void)sprintf(mls,"%03ld,%03ld,%03ld",mil,(num-mil*1000000)/1000,num%1000);
   ln=strlen(mls)-1;
   while (p<ln && (mls[p]=='0' || mls[p]==',')) p++;
   return(&mls[p]);
}

void check_value(val,mini,maxi,prompt)
long val;
int mini;
long maxi;
char *prompt;
{
   if (val<mini || val>maxi)
   {
      (void)fprintf(stderr,"Error: Wrong value (%s) for %s !\n",mills(val),prompt);
      (void)fprintf(stderr,"       It should be between %d and %s.\n",mini,mills(maxi));
      exit(1);
   }
}

void init_fact()
{
   int z; 
   double mult=1.0;
   for (z=1;z<=upper;z++) { mult*=z; fact[z]=mult; }
   num_combs=(int)calc_combs(min_match,drawn); /* e.g. 3 from 6 = 20 */
   tot_combs=calc_combs(drawn,upper); /* e.g. 6 from 49 = 13,983,816 */
   check_value(calc_combs(3,drawn),3,MAX_COMBS,
               "3 from MAX_DRAWN (MAX_COMBS wrong in source)");
   check_value(losing_rep,0,tot_combs,"-l option");
   (void)printf("A %d from %d lottery has %s combinations\n\n",
                 drawn,upper,mills(tot_combs));
}

void init_combs()
{
   int a,b,c;
   int gen[4];
   (void)printf("\nVerify %s - a %d-match combinations verifier for a %d from %d lottery\n",
          VERSION,min_match,drawn,upper);
   (void)printf("(C) Richard K. Lloyd and Connect Internet Solutions Limited 1996-2002\n");
   (void)printf("    <webmaster@lottery.merseyworld.com>\n\n");
   (void)printf("For information about the UK National Lottery, see:\n");
   (void)printf("http://lottery.merseyworld.com/\n\n");
   init_fact();
   /* Reset combination count to zero. One of the few areas of code needing
      conditional statement for min_match */
   if (min_match==2)
   for (a=1;a<=upper-1;a++)
   for (b=a+1;b<=upper;b++)
   { combs[a][b][0]='\0'; }
   else
   for (a=1;a<=upper-2;a++)
   for (b=a+1;b<=upper-1;b++)
   for (c=b+1;c<=upper;c++)
   { combs[a][b][c]='\0'; }
   /* Generate all num_combs combinations of min_match from drawn as indexes */
   for (a=1;a<=min_match;a++) { gen[a]=a; }
   for (a=1;a<=num_combs;a++)
   {
      /* This next lets min_match=2 index correctly later on and avoids
         conditional code */
      perms[a][0]=0;
      for (b=1;b<=min_match;b++) { perms[a][b]=gen[b]; }
      b=min_match;
      while (b && ++gen[b]>drawn-(min_match-b)) { b--; }
      if (b>0 && b<min_match) for (c=min_match;c>b;c--) { gen[c]=gen[b]+c-b; }
   }
   freq=min_match-1; /* Reset report frequency */
}

void balls_line()
{
   int x,y,z,scptr=0,ins,sorted=1,newcomb=0;
   for (x=1;x<=drawn;x++)
   {
      /* This code acts like sscanf() should really - allowing me to traverse
         to the next non-space item in a string. Why doesn't sscanf() return a
         (char *) to the char after the last char of the object read in ? */
      while ((buff[scptr]==' ' || buff[scptr]=='\t') && buff[scptr]!='\n')
      { scptr++; }
      (void)sscanf(&buff[scptr],"%d",&y);
      while ((buff[scptr]!=' ' && buff[scptr]!='\t') && buff[scptr]!='\n')
      { scptr++; }
      /* Don't allow anything that evals to zero or exceeds upper !! */
      if (y<1 || y>upper)
      {
         (void)fprintf(stderr,"Ticket #%s contains a bad number (%d)\n",mills(tcount),y);
         exit(1);
      }
      /* Need the numbers in sorted order and some numbskulls may be dumb
         enough to provide a file with tickets containing non-ascending
         numbers. */
      ins=x;
      for (z=1;z<x && ins==x;z++) 
      {
         if (y<balls[z]) ins=z;
         else
         if (balls[z]==y)
         {
            (void)fprintf(stderr,"Ticket #%s has duplicate numbers (%d)\n",mills(tcount),y);
            exit(1);
         }
      }
      for (z=x;z>ins;z--)
      {
         balls[z]=balls[z-1]; sorted=0;
      }
      balls[ins]=y;
   }

   if (!sorted)
     (void)fprintf(stderr,"WARNING: Ticket #%s is unsorted (I sorted it for you)\n",mills(tcount));

   /* Neat this bit - the permutation array simply let us index straight into
      the balls array, which is then used to increment the combination
      count */
   for (x=1;x<=num_combs;x++)
   {
      int b1=balls[perms[x][1]],
          b2=balls[perms[x][2]],
          b3=balls[perms[x][3]];
      /* Chars only go up to 255, so check for that */
      y=(int)combs[b1][b2][b3]; if (!y) newcomb=1;
      if (y<255) combs[b1][b2][b3]=(char)(y+1);
   }

   /* No new combinations covered, so this ticket must be redundant ! */
   if (!newcomb)
   {
      (void)fprintf(stderr,"EXTREME WARNING: Ticket #%s ( ",mills(tcount));
      for (x=1;x<=drawn;x++)
      { (void)fprintf(stderr,"%2d ",balls[x]); }
      (void)fprintf(stderr,")\ncovers no new combinations - this makes it redundant !\n\n");
   }
}

void scan_combs(c1,c2,c3)
int c1,c2,c3;
{
   char comb_str[32];
   int c=(int)combs[c1][c2][c3];
   if (c3) (void)sprintf(comb_str,"(%2d %2d %2d)",c1,c2,c3);
   else (void)sprintf(comb_str,"(%2d %2d)",c1,c2);
   cover[c]++;
   if (c>bs) bs=c;
   if (c<l) l=c;
   switch (c)
   {
      case 0:
         missed++;
         break;
      case 2:
         (void)printf("Covered combination %s twice\n",comb_str);
      case 1:
         covered++;
         break;
      default:
         (void)printf("Covered combination %s %d times\n",comb_str,c);
         covered++;
         break;
   }
}

void summary()
{
   int x,y,z;
   long totcombs,cov;
   for (x=0;x<=255;x++) { cover[x]=0; }
   (void)printf("Total tickets read in: %s\n\n",mills(tcount));
   if (min_match==2)
      for (x=1;x<=upper-1;x++)
      for (y=x+1;y<=upper;y++)
      { scan_combs(x,y,0); }
   else
      for (x=1;x<=upper-2;x++)
      for (y=x+1;y<=upper-1;y++)
      for (z=y+1;z<=upper;z++)
      { scan_combs(x,y,z); }
   totcombs=missed+covered;
   (void)printf("\nOverall %d-match combination coverage summary:\n",min_match);
   for (x=bs;x>=l;x--)
   {
      if ((cov=cover[x]))
      {
         (void)printf("Covered ");
         switch (x)
         {
             case 0:
                (void)printf("  never ");
                break;
             case 1:
                (void)printf("  once  ");
                break;
             case 2:
                (void)printf("  twice ");
                break;
             default:
                (void)printf("%2d times",x);
         }
         (void)printf(" : %6s\n",mills(cov));
      }
   }
   (void)printf("Total: %9s / ",mills(covered));
   (void)printf("%6s (%.1f%%)\n",mills(totcombs),
                (double)covered*100.0/(double)totcombs);
}

void out_result(prompt,amount,tot)
char *prompt;
long amount,tot;
{
   (void)printf("%s combinations: %10s (%5.1f%%)\n",prompt,mills(amount),(double)amount*100.0/(double)tot);
}

void user_report(tks,out)
long tks;
int out;
{
   /* Give the user an update so they don't get bored. Even with
      this fast code, it still takes a fair time to run, especially on slow
      machines. */
   int tl,tx;
   long el=time((time_t *)NULL)-st_time;
   if (el<1) el=1; /* Not catching me with a divide by zero ! */
   /* Only display ticket combination if in the middle of the checking */
   if (out)
      for (tx=1;tx<=drawn;tx++)
      { (void)fprintf(stderr,"%2d ",pt[tx]); }
   else (void)fprintf(stderr,"%*s",-(drawn*3),"     Finished");
   tl=(tot_combs-tks)/(tks/el);
   (void)fprintf(stderr,"%12s  %3ld:%02ld ",mills(tks),el/60,el%60);
   (void)fprintf(stderr,"%10s %4d:%02d\n",mills(tks/el),tl/60,tl%60);
   /* Auto-adjust report frequency depending on how many seconds are
      left. Don't report in the last "cutoff" seconds to make sure full speed
      is gained, particularly for machines that can do the entire thing in
      about 30 seconds. Yes, printing the report radically reduces the
      speed of the program if it's done a lot of times, which happens
      in the final combinations. */
   if (cutoff && freq>1 && tl<cutoff)
   {
      (void)fprintf(stderr,"[Reporting switched off during the final ");
      if (cutoff>1) (void)fprintf(stderr,"%d seconds]\n",cutoff);
      else (void)fprintf(stderr,"second]\n");
   }
   if (cutoff) freq=(int)(tl/cutoff)+1; else freq=1;
   /* Losing ticket output causes freq get big, so trim it just in case */
   if (freq>drawn-min_match) freq=drawn-min_match;
   done_rep=1;
}

void intense()
/* This routine will take at least 15 seconds to execute, even on the fastest
   machines around (HP 735/125, Dec Alpha and SUN Ultras). It scans all (14
   million for the UK) ticket combinations and sees if at least one of the (20
   per ticket for a 6-ball lottery) 3-match combinations each of those tickets
   covers are picked up by the tickets supplied in the data file.

   There may be a quicker algorithm to achieve this, but I've hand-crafted the
   code for this way to be as fast as possible. If you can't manage 500,000
   combinations per second on your machine, then you need to upgrade ! */
{
   int x,ti=drawn;
   long losing=0,winning=0,nt;
   char fl;
   /* Reset combination pointers, including pt[0], which is used when
      min_match=2 (because perms[x][3]=0) */
   for (x=0;x<=drawn;x++) { pt[x]=x; }
   (void)fprintf(stderr,"\n%*s    Covered   Elapsed    Speed    To Go\n",
                 -(drawn*3),"    Combination");
   /* Record start time in seconds */
   st_time=time((time_t *)NULL);
   while (ti)
   {
      /* This next bit of code is so quick, it's embarrassing ! */
      fl='\0';
      for (x=1;x<=num_combs && fl=='\0';x++)
      {
         /* I love this bit... bracket overload or what ? */
         fl=combs[pt[perms[x][1]]][pt[perms[x][2]]][pt[perms[x][3]]];
      }
      if (fl=='\0')
      {
         if (++losing<=losing_rep)
         {
            for (x=1;x<=drawn;x++) { (void)printf("%2d ",pt[x]); }
           (void)printf("   Losing combination #%s\n",mills(losing));
         }
      } else winning++;
      /* Completed a combination so skip to the next one */
      ti=drawn;
      while (ti && ++pt[ti]>upper-(drawn-ti)) { ti--; }
      if (ti>0 && ti<drawn)
      {
         /* Not just an increment, so do a pointer reset */
         for (x=drawn;x>ti;x--) { pt[x]=pt[ti]+x-ti; }
         /* Give progress report if not near the end and also have tweaked
            an appropriate index */
         if (ti<freq) user_report(winning+losing,1);
         else
         /* May be a dead slow machine (PC...ha ha) and might display no
            report for 10 or more seconds, so force a one-off after 3 seconds
            if there hasn't been one already */
         if (!done_rep && ti==freq)
            if (time((time_t *)NULL)-st_time>2)
              user_report(winning+losing,1);
      }
   }
   nt=winning+losing;
   user_report(nt,0);
   (void)printf("\n");
   out_result("Losing ",losing,nt);
   out_result("Winning",winning,nt);
   (void)printf("Total   combinations: %10s (for a %d from %d lottery)\n",
          mills(nt),drawn,upper);
}

void scan_file(fhand)
FILE *fhand;
{
   while (fgets(buff,512,fhand)!=(char *)NULL)
   {
      if (buff[0]!='\n' && buff[0]!='#')
      {
         tcount++;
         balls_line();
      }
   }
}

void usage()
{
   (void)fprintf(stderr,"Syntax: verify [-b <balls>] [-c <report cutoff>] [-h] [-l <losing combs>]\n");
   (void)printf("                       [-m <matches>] [-t <ticket size>] [<filename>]\n\n");
   (void)fprintf(stderr,"-b specifies the number of balls in the lottery machine (default = %d)\n",upper);
   (void)fprintf(stderr,"-c will stop progress reporting with the specified seconds to go (default = %d)\n",DEF_CUTOFF);
   (void)fprintf(stderr,"-h displays this syntax message\n");
   (void)fprintf(stderr,"-l specifies the max losing combinations reported (default = %d)\n",DEF_LOSING_REP);
   (void)fprintf(stderr,"-m indicates the number of guaranteed matches (2 or 3, default = %d)\n",min_match);
   (void)fprintf(stderr,"-t sets how many numbers are on a ticket (default = %d)\n\n",drawn);
   (void)fprintf(stderr,"If no filename is specified, standard input is read.\n");
   exit(1);
}

void command_line_params(argc,argv)
int argc;
char **argv;
{
   int param;
   while ((param=getopt(argc,argv,"?b:c:hl:m:t:"))!=EOF)
   {
      switch (param)
      {
         case 'b': upper=atoi(optarg); /* Set number of balls */
                   break;
         case 'c': cutoff=atoi(optarg); /* Report cutoff point in secs */
                   break;
         case 'l': losing_rep=atol(optarg); /* Max losing combs reported */
                   break;
         case 'm': min_match=atoi(optarg); /* Set guaranteed matches */
                   break;
         case 't': drawn=atoi(optarg); /* Set numbers on a ticket */
                   break;
         case 'h': /* Syntax request */
         case '?': usage(); /* Bad option */
                   break;
      }
   }
   check_value((long)min_match,2,(long)3,"guaranteed matches");
   check_value((long)drawn,min_match+1,(long)MAX_DRAWN,"numbers on a ticket");
   check_value((long)upper,drawn+1,(long)MAX_UPPER,"balls in the machine");
   check_value((long)cutoff,0,(long)(MAX_CUTOFF),"report cutoff (in seconds)");
}

/* Yes, main() is an int-returning function. "Not a lot of people know that,"
   it says here in the Michael Caine Guide To Programming */
int main(argc,argv)
int argc;
char **argv;
{
   command_line_params(argc,argv);
   init_combs();
   if (argc==optind)
   {
      (void)fprintf(stderr,"NOTE: Now reading from standard input.\n");
      (void)fprintf(stderr,"      Specify a filename with the \"verify\" command to read from that file.\n");
      scan_file(stdin);
   }
   else
   {
      if ((inhand=fopen(argv[optind],"r"))==(FILE *)NULL)
      {
         (void)fprintf(stderr,"Cannot open the file %s !\n",argv[optind]);
         exit(1);
      }
      scan_file(inhand);
      (void)fclose(inhand);
   }
   summary();
   intense();
   return(0); /* Not many programs do this ! */
}
