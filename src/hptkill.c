/*
 *      hptkill - areas killer for high Portable Tosser (hpt)
 *      by Serguei Revtov 2:5021/11.10 || 2:5021/19.1
 *      Some code was taken from hpt/src/areafix.c
 *
 * This file is part of HPT.
 *
 * HPT is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HPT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPT; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************
 * $Id$
 */

#include <stdio.h>
#include <ctype.h>

#ifdef UNIX
#include <unistd.h>
#include <strings.h>
#else
#include <io.h>
#endif

#include <fcntl.h>
#ifdef __EMX__
#include <share.h>
#include <sys/types.h>
#endif
#include <sys/stat.h>

#ifdef __WATCOMC__
#include <fcntl.h>
#define AW_S_ISDIR(a) (((a) & S_IFDIR) != 0)
#include <process.h>
#include <dos.h>
#endif
#include <fcntl.h>
#include <errno.h>

#if defined(__TURBOC__) || defined(__IBMC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))

#include <io.h>
#include <fcntl.h>

#if !defined(S_ISDIR)
#define S_ISDIR(a) (((a) & S_IFDIR) != 0)
#endif

#endif

#include <time.h>

#include <smapi/msgapi.h>
#include <smapi/progprot.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <fidoconf/xstr.h>

#include <string.h>

#if defined ( __WATCOMC__ )
#include <smapi/prog.h>
#include <share.h>
#endif

#include <stdlib.h>

s_fidoconfig *config;

FILE *outlog;
char *version = "1.10.2-release";

typedef enum senduns { eNobody, eFirstLink, eNodes, eAll} e_senduns;

e_senduns sendUnSubscribe = eNodes;
int     delFromConfig = 0;
int     eraseBase = 1;
int     killSend = 0;
int     killPass = 0;
int     createDupe = 0;

typedef struct xmsgtxt {
	XMSG xmsg;
	char *text;
} s_xmsgtxt;


void usage(void) {

    fprintf(outlog, "hptkill %s\n", version);
    fprintf(outlog, "Areas killing utility\n");
    fprintf(outlog, "Usage:\n hptkill [options] [areaNameMask ...]\n");
    fprintf(outlog, "   -c config-file - specify alternate config file\n");
    fprintf(outlog, "   -1 - send unsubscribe message to first link only\n");
    fprintf(outlog, "   -n - don't send unsubscribe message\n");
    fprintf(outlog, "   -a - send unsubscribe message all subscribed links\n");
    fprintf(outlog, "   -d - delete area from config\n");
    fprintf(outlog, "   -s - save (don't erase) message & dupe bases\n");
    fprintf(outlog, "   -f file - read areas list from file in addition to args\n");
    fprintf(outlog, "   -f -    - read areas list from stdin in addition to args\n");
    fprintf(outlog, "   -k - set Kill/Sent attribute to messages for links\n");
    fprintf(outlog, "   -p - find & kill passthrough echoareas with <=1 links\n");
    fprintf(outlog, "   -pp - same as -p including paused links\n");
    fprintf(outlog, "   -y - find & kill ANY echoareas with <=1 links\n");
    fprintf(outlog, "   -yp - same as -y including paused links\n");
    fprintf(outlog, "   -o days - kill passthrough area with dupebase older 'days' days\n");
    fprintf(outlog, "   -O days - same as -o but kill areas without dupebases\n");
    fprintf(outlog, "   -l file - with -o/-O write to file list of areas without dupebase\n");
    fprintf(outlog, "   -C - create empty dupebase if it doesn't exist\n");
    fprintf(outlog, "\nDefault settings:\n");
    fprintf(outlog, " -  send unsubscribe message to subcribed nodes only\n");
    fprintf(outlog, " -  leave config unchanged\n");
    fprintf(outlog, " -  erase message & dupe bases\n");
    exit(-1);

}

void exit_err(char *text)
{
    fprintf(outlog, "exiting: %s\n", text);
    exit(1);
}


#if defined(__TURBOC__) || defined(__IBMC__) || defined(__WATCOMC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))

int truncate(const char *fileName, long length)
{
    int fd = open(fileName, O_RDWR | O_BINARY);
    if (fd != -1) {
	lseek(fd, length, SEEK_SET);
	chsize(fd, tell(fd));
	close(fd);
	return 1;
    };
    return 0;
}

int fTruncate( int fd, long length )
{
    if( fd != -1 )
	{
	    lseek(fd, length, SEEK_SET);
	    chsize(fd, tell(fd) );
	    return 1;
	}
    return 0;
}

#endif

#ifdef __MINGW32__
int fTruncate (int fd, long length)
{
    if( fd != -1 )
	{
	    lseek(fd, length, SEEK_SET);
	    chsize(fd, tell(fd) );
	    return 1;
	}
    return 0;
}
#endif

int delareafromconfig(char *fileName, s_area *area) {
    FILE *f;
    char *cfgline, *token, *areaName, *buff;
    long pos=-1, lastpos, endpos, len;

    areaName = area->areaName;

    if (init_conf(fileName)) return 1;

    while ((buff = configline()) != NULL) {
	buff = trimLine(buff);
	buff = stripComment(buff);
	if (buff[0] != 0) {
	    buff = cfgline = shell_expand(buff);
	    token = strseparate(&cfgline, " \t");
	    if (stricmp(token, "echoarea")==0) {
		token = strseparate(&cfgline, " \t");
    		if (stricmp(token, areaName)==0) {
        	    fileName = sstrdup(getCurConfName());
        	    pos = getCurConfPos();
        	    break;
    		}
	    }
	}
	nfree(buff);
    }
    close_conf();
    if (pos == -1) return 1; // impossible
    nfree(buff);

    if ((f=fopen(fileName,"r+b")) == NULL)
	{
	    fprintf(outlog, "\ncannot open config file %s \n", fileName);
	    nfree(fileName);
	    return 1;
	}
    fseek(f, pos, SEEK_SET);
    cfgline = readLine(f);
    if (cfgline == NULL) {
	fclose(f);
	nfree(fileName);
	return 1;
    }

    fprintf(outlog, " %s...", fileName);

    lastpos = ftell(f);
    fseek(f, 0, SEEK_END);
    endpos = ftell(f);
    if (endpos>lastpos) {
	buff = (char*) smalloc((size_t) (endpos-lastpos));
	memset(buff, '\0', (size_t) (endpos-lastpos));
	fseek(f, lastpos, SEEK_SET);
	len = fread(buff, sizeof(char), (size_t) endpos-lastpos, f);
	fseek(f, pos, SEEK_SET);
	fwrite(buff, sizeof(char), (size_t) len, f);
	nfree(buff);
    } else {
	len=0;
    }
#if defined(__WATCOMC__) || defined(__MINGW32__)
    fflush( f );
    fTruncate( fileno(f), pos+len);
    fflush( f );
#else
    truncate(fileName, pos+len);
#endif
    nfree(cfgline);
    nfree(fileName);
    fclose(f);
    return 0;
}

int putMsgInArea(s_area *echo, XMSG  *xmsg, char *text)
{
    char *ctrlBuff, *textStart, *textWithoutArea;
    UINT textLength;
    HAREA harea;
    HMSG  hmsg;
    char *slash;
    int rc = 0;

    // create Directory Tree if necessary
    if (echo->msgbType == MSGTYPE_SDM)
	_createDirectoryTree(echo->fileName);
    else if (echo->msgbType==MSGTYPE_PASSTHROUGH) {
	fprintf(outlog, "Can't put message to passthrough area %s!",
		echo->areaName);
	return rc;
    } else {
	// squish or jam area
	slash = strrchr(echo->fileName, PATH_DELIM);
	if (slash) {
	    *slash = '\0';
	    _createDirectoryTree(echo->fileName);
	    *slash = PATH_DELIM;
	}
    }

    harea = MsgOpenArea((UCHAR *) echo->fileName, MSGAREA_CRIFNEC, (word)(echo->msgbType));
    if (harea != NULL) {
	hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);
	if (hmsg != NULL) {

	    textWithoutArea = text;
	    textLength = strlen(text);

	    ctrlBuff = (char *) CopyToControlBuf((UCHAR *) textWithoutArea,
						 (UCHAR **) &textStart,
						 &textLength);
	    // textStart is a pointer to the first non-kludge line

	    MsgWriteMsg(hmsg, 0, xmsg, (byte *) textStart, (dword) strlen(textStart), (dword) strlen(textStart), (dword)strlen(ctrlBuff), (byte *)ctrlBuff);

	    MsgCloseMsg(hmsg);
	    nfree(ctrlBuff);
	    rc = 1;

	} else {
	    fprintf(outlog, "Could not create new msg in %s!", echo->fileName);
	} /* endif */

	MsgCloseArea(harea);

    } else {
	fprintf(outlog, "Could not open/create Area %s!", echo->fileName);
    } /* endif */
    return rc;
}


int makeRequestToLink (char *areatag, s_link *link) {
    s_xmsgtxt  *xmsgtxt;
    XMSG *xmsg;
    time_t curTime;
    static time_t preTime=0L;
    struct tm *date;
    union stamp_combo dosdate;

    if (link->hisAka.point)
	fprintf(outlog, "  Make message for %u:%u/%u.%u...",
		link->hisAka.zone ,
		link->hisAka.net  ,
		link->hisAka.node ,
		link->hisAka.point);
    else
	fprintf(outlog, "  Make message for %u:%u/%u...",
		link->hisAka.zone ,
		link->hisAka.net  ,
		link->hisAka.node);

    if (link->msg == NULL) {
	xmsgtxt = (s_xmsgtxt *) scalloc( 1, sizeof(s_xmsgtxt));
	link->msg = xmsgtxt;

	xmsg = &(xmsgtxt->xmsg);

	xmsg->orig.zone  = link->ourAka->zone ;
	xmsg->orig.net   = link->ourAka->net  ;
	xmsg->orig.node  = link->ourAka->node ;
	xmsg->orig.point = link->ourAka->point;
	xmsg->dest.zone  = link->hisAka.zone ;
	xmsg->dest.net   = link->hisAka.net  ;
	xmsg->dest.node  = link->hisAka.node ;
	xmsg->dest.point = link->hisAka.point;
	strcpy( (char*)(xmsg->from), (char*)(config->sysop) );

	strcpy( (char*)(xmsg->to), (char*)(link->RemoteRobotName ? link->RemoteRobotName : "AreaFix") );
	strcpy( (char*)(xmsg->subj), (char*)(link->areaFixPwd ? link->areaFixPwd : "\0") );

	xmsg->attr = MSGLOCAL|MSGPRIVATE;

	if (killSend) xmsg->attr |= MSGKILL;

	if (xmsg->orig.point) xscatprintf(&(xmsgtxt->text), "\001FMPT %d\r", xmsg->orig.point);
	if (xmsg->dest.point) xscatprintf(&(xmsgtxt->text), "\001TOPT %d\r", xmsg->dest.point);

	curTime = time(NULL);
	while (curTime == preTime) {
	    sleep(1);
	    curTime = time(NULL);
	}
	preTime = curTime;

	if (link->ourAka->point)
	    xscatprintf(&(xmsgtxt->text), "\001MSGID: %u:%u/%u.%u %08lx\r",
			link->ourAka->zone,
			link->ourAka->net,
			link->ourAka->node,
			link->ourAka->point,
			(unsigned long) curTime);
	else
	    xscatprintf(&(xmsgtxt->text), "\001MSGID: %u:%u/%u %08lx\r",
			link->ourAka->zone,
			link->ourAka->net,
			link->ourAka->node,
			(unsigned long) curTime);

	date = localtime(&curTime);

	fts_time(xmsg->__ftsc_date, date);

	ASCII_Date_To_Binary(xmsg->__ftsc_date, (union stamp_combo *) &(xmsg->date_written));
	TmDate_to_DosDate(date, &dosdate);
	xmsg->date_arrived = dosdate.msg_st;
    } else {
	fprintf(outlog, "adding...");
	xmsgtxt = (s_xmsgtxt *) link->msg;
    }

    xscatprintf(&(xmsgtxt->text), "-%s\r", areatag);

    fprintf(outlog, "done\n");

    return 0;
}

char *createDupeFileName(s_area *area) {
    char *name=NULL, *afname, *retname=NULL;

    if (!area->DOSFile) {
	xstrcat(&name, area->areaName);
    } else {
	if (area->fileName)
	    xstrcat(&name,(afname=strrchr(area->fileName,PATH_DELIM))
		    ? afname+1 : area->fileName);
	else xstrcat(&name, "passthru");
    }

    switch (config->typeDupeBase) {
    case hashDupes:
	xstrcat(&name,".dph");
	break;
    case hashDupesWmsgid:
	xstrcat(&name,".dpd");
	break;
    case textDupes:
	xstrcat(&name,".dpt");
	break;
    case commonDupeBase:
	break;
    }

    if (config->areasFileNameCase == eUpper)
	name = strUpper(name);
    else
	name = strLower(name);

    xstrscat(&retname, config->dupeHistoryDir, name, NULL);
    nfree(name);

    return retname;
}

void delete_area(s_area *area)
{
    char *an = area->areaName;
    unsigned int i;
    int rc;

    fprintf(outlog, "Kill area %s\n", an);

    switch (sendUnSubscribe) {

    case eNobody:
	break;

    case eFirstLink:
	makeRequestToLink(an, area->downlinks[0]->link);
	break;

    case eNodes:
	for (i=0; i<area->downlinkCount; i++) {
	    if (area->downlinks[i]->link->hisAka.point == 0)
		makeRequestToLink(an, area->downlinks[i]->link);
	}
	break;

    case eAll:
	for (i=0; i<area->downlinkCount; i++) {
	    makeRequestToLink(an, area->downlinks[i]->link);
	}
	break;
    }

    /* delete msgbase and dupebase for the area */
    if (eraseBase) {

	if (area->msgbType!=MSGTYPE_PASSTHROUGH) {
	    fprintf(outlog, "  Removing base of %s...", an);
	    rc=MsgDeleteBase(area->fileName, (word) area->msgbType);
	    fprintf(outlog, "%s\n", rc ? "ok" : "unsuccessful");
	}


	if (area->dupeCheck != dcOff) {
	    char *dupename = createDupeFileName(area);
	    if (dupename) {
		fprintf(outlog, "  Removing dupes of %s...", an);
		rc=unlink(dupename);
		fprintf(outlog, "%s\n", rc==0 ? "ok" : "unsuccessful");
		nfree(dupename);
	    }
	}
    }

    /* remove area from config-file */
    if (delFromConfig) {
	fprintf(outlog, "   deleting from config");
	if (delareafromconfig (getConfigFileName(),  area) != 0)
	    fprintf(outlog, " ERROR!\n");
	else
	    fprintf(outlog, " ok\n");

	/* delete the area from in-core config */
	for (i=0; i<area->downlinkCount; i++)
	    nfree(area->downlinks[i]);
	nfree(area->downlinks);
	area->downlinkCount = 0;
	for (i=0; i<config->echoAreaCount; i++)
	    if (stricmp(config->echoAreas[i].areaName, an)==0)
		break;
	if (i<config->echoAreaCount && area==&(config->echoAreas[i])) {
	    nfree(area->areaName);
	    nfree(area->fileName);
	    nfree(area->description);
	    nfree(area->group);
	    for (; i<config->echoAreaCount-1; i++)
		memcpy(&(config->echoAreas[i]), &(config->echoAreas[i+1]),
		       sizeof(s_area));
	    config->echoAreaCount--;
	}
    }
    fprintf(outlog, "done\n"); // can't use an here - freed
}


int main(int argc, char **argv) {

    int i, j, k;
    struct _minf m;
    char **areas = NULL;
    char *needfree = NULL;
    char *line = NULL;
    int nareas=0;
    int found = 0;
    FILE *f = NULL;
    s_xmsgtxt *xmsgtxt = NULL;
    s_link *link = NULL;
    int killed = 0;
    int checkPaused = 0;
    int killNoLink = 0;
    int killLowLink = 0;
    int killOld = 0;
    int killWithoutDupes = 0;
    int delArea = 0;
    time_t oldest = 0;
    s_area *area = NULL;
    struct stat stbuf;
    char *listNoDupeFile = NULL;
    FILE *fNoDupe = NULL;
    char *dupename = NULL;
    char *cfgfile = NULL;


    outlog=stdout;

    setbuf(outlog, NULL);

    for (i=1; i<argc; i++) {
	if ( argv[i][0] == '-' ) {
	    switch (argv[i][1])
		{
		case 'c': /* alternate config file */
                    if ( ++i<argc ) {
                      cfgfile = argv[i];
                    } else {
                      fprintf( stderr, "Parameter required after -c\n");
                      usage();
                    }
                    break;
		case '1': /* send unsubscribe message to first link only */
		    sendUnSubscribe = eFirstLink;
		    break;

		case 'n': /* don't send unsubscribe message */
		case 'N':
		    sendUnSubscribe = eNobody;
		    break;

		case 'a': /* send unsubscribe message all links */
		case 'A':
		    sendUnSubscribe = eAll;
		    break;

		case 'd': /* delete area from config */
		case 'D':
		    delFromConfig = 1;
		    break;

		case 's': /* save (don't erase) message & dupe bases */
		case 'S':
		    eraseBase = 0;
		    break;

		case 'f': /* read areas list from file */
		case 'F':
		    i++;

		    if ( argv[i] == NULL || argv[i][0] == '\0') {
			usage();
			exit(-1);
		    }

		    if (strcmp(argv[i], "-") == 0) {
			f=stdin;
		    } else {
			if ( !(f=fopen(argv[i], "r"))) {
			    fprintf(outlog, "%s: Can't open file '%s'\n", argv[0], argv[i]);
			    exit(-1);
			}
		    }

		    while (!feof(f)) {
			line = readLine(f);

			if (line) {
			    nareas++;
			    areas = (char **)srealloc ( areas, nareas*sizeof(char *));
			    areas[nareas-1] = line;
			    needfree = (char *)srealloc ( needfree, nareas*sizeof(char));
			    needfree[nareas-1] = 1;
			}

		    }

		    if (f != stdin) fclose(f);
		    break;

		case 'k': /* set Kill/Sent attribute to messages for links */
		case 'K':
		    killSend = 1;
		    break;

		case 'p': /* kill passthrough areas with 1 link*/
		case 'P':
		    killNoLink++;
		    killPass++;
		    if (argv[i][2]=='p' || argv[i][2]=='P') checkPaused++;
		    break;

		case 'y': /* kill ANY areas with <=1 link*/
		case 'Y':
		    killLowLink++;
		    if (argv[i][2]=='p' || argv[i][2]=='P') checkPaused++;
		    break;

		case 'o': /* kill passthrough area with dupebase older 'days' days */
		case 'O':
		    if (argv[i][1]=='O') killWithoutDupes++;
		    i++;
		    if ( argv[i] == NULL || argv[i][0] == '\0') {
			usage();
			exit(-1);
		    }
		    killPass++;
		    killOld++;
		    oldest = time(NULL) - atoi(argv[i]) * 60*60*24;
		    break;

		case 'l': /* write list of areas without dupebase to file  */
		case 'L':
		    i++;
		    if ( argv[i] == NULL || argv[i][0] == '\0') {
			usage();
			exit(-1);
		    }
		    listNoDupeFile = argv[i];
		    break;

		case 'C': /* create empty dupebase if it doesn't exist */
		    createDupe++;
		    break;

		default:
		    usage();
		    exit(-1);
		}
	} else {
	    // AreaName(s) specified by args
	    nareas++;
	    areas = (char **)srealloc ( areas, nareas*sizeof(char *));
	    areas[nareas-1] = argv[i];
	    needfree = (char *)srealloc ( needfree, nareas*sizeof(char));
	    needfree[nareas-1] = 0;
	}
    }

    if (nareas == 0) {
	if (killPass || killLowLink) {
	    nareas++;
	    areas = (char **)srealloc ( areas, nareas*sizeof(char *));
	    areas[nareas-1] = "*";
	    needfree = (char *)srealloc ( needfree, nareas*sizeof(char));
	    needfree[nareas-1] = 0;
	} else {
	    if (!createDupe) {
		usage();
		exit (-1);
	    }
	}
    }

    fprintf(outlog,"hptkill %s\n", version);

    if( cfgfile && cfgfile[0] )
           config = readConfig(cfgfile);
    else   config = readConfig(getConfigFileName());

    if (!config) {
	fprintf(outlog, "Could not read fido config\n");
	return (1);
    }

    m.req_version = 0;
    m.def_zone = (UINT16) config->addr[0].zone;
    if (MsgOpenApi(&m) != 0) fprintf(outlog, "MsgApiOpen Error");

    for ( j=0; j<nareas; j++) {
	found = 0;

	for (i=0, area = config->echoAreas; (unsigned int)i < config->echoAreaCount; i++, area++) {
	    if (patimat(area->areaName, areas[j])==1){

		delArea = 0;
		if (killPass==0 && killLowLink==0) delArea++;
		else if ((area->msgbType & MSGTYPE_PASSTHROUGH) == MSGTYPE_PASSTHROUGH) {
		    if (killNoLink) {
			if (area->downlinkCount <= 1) delArea++;
			else if (checkPaused) {
			    delArea = 2; // if two links w/o pause - leave untouched
			    for (k=0; (unsigned int)k < area->downlinkCount && delArea; k++) {
				if (area->downlinks[k]->link->Pause == 0) delArea--;
			    }
			}
		    }
		    if (killOld && !delArea && area->dupeCheck != dcOff) {
			dupename = createDupeFileName(area);
			if (dupename) {
			    if (stat(dupename, &stbuf)==0) {
				if (stbuf.st_mtime < oldest) delArea++;
			    } else {
				if (killWithoutDupes) {
				    delArea++;
				}
				if (listNoDupeFile) {
				    if (!fNoDupe) {
					if (!(fNoDupe=fopen (listNoDupeFile, "a"))) {
					    fprintf (stderr, "Can't open file '%s' for appending\n", listNoDupeFile);
					}
				    }
				    if (fNoDupe) fprintf (fNoDupe, "%s\n", area->areaName);

				}
			    }
			    nfree(dupename);
			}
		    }
		}
		if (killLowLink) {
	            if (area->downlinkCount <= 1) delArea++;
		    else if (checkPaused) {
		        delArea = 2; // if two links w/o pause - leave untouched
		        for (k=0; k < area->downlinkCount && delArea; k++) {
		          if (area->downlinks[k]->link->Pause == 0) delArea--;
		        }
		    }
		}
		if (delArea) {
		    delete_area(area);
		    killed++;
		    found++;
		    if (delFromConfig) { // Area is removed from areas array!
			i--;
			area--;
		    }

		}
	    }
	}

	if (killPass==0 && killLowLink==0) {
	    for (i=0, area=config->localAreas; (unsigned int)i < config->localAreaCount; i++, area++) {
		if (patimat(area->areaName, areas[j])==1){
		    delete_area(area);
		    killed++;
		    found++;
		    if (delFromConfig) { // Area is removed from areas array!
			i--;
			area--;
		    }
		}
	    }

	    if (!found) fprintf(outlog, "Couldn't find area \"%s\"\n", areas[j]);
	}
    }

    if (fNoDupe) fclose (fNoDupe);

    if (killed) fprintf(outlog, "\n");
    // Put mail for links to netmail
    for (i=0; (unsigned int)i < config->linkCount; i++) {
	if (config->links[i].msg) {
	    link = &(config->links[i]);
	    if (link->hisAka.point)
		fprintf(outlog, "Write message for %u:%u/%u.%u...",
			link->hisAka.zone ,
			link->hisAka.net  ,
			link->hisAka.node ,
			link->hisAka.point);
	    else
		fprintf(outlog, "Write message for %u:%u/%u...",
			link->hisAka.zone ,
			link->hisAka.net  ,
			link->hisAka.node);
	    xmsgtxt = (s_xmsgtxt *) link->msg;
	    putMsgInArea(&(config->netMailAreas[0]), &(xmsgtxt->xmsg), xmsgtxt->text);
	    nfree(link->msg);
	    fprintf(outlog, "done\n");
	}
    }

    for ( j=0; j<nareas; j++) if (needfree[nareas-1]) nfree(areas[j]);
    if (needfree) nfree(needfree);
    if (areas) nfree(areas);

    if (killed && config->echotosslog) {
	f=fopen(config->echotosslog, "a");
	if (f==NULL) {
	    fprintf(outlog, "\nCould not open or create EchoTossLogFile\n");
	} else {
	    fprintf(f,"%s\n", config->netMailAreas[0].areaName);
	    fclose(f);
	}
    }

    if (createDupe) {
	for (i=0, area = config->echoAreas; (unsigned int)i < config->echoAreaCount; i++, area++) {
	    dupename = createDupeFileName(area);
	    if (stat(dupename, &stbuf)!=0) {
		fprintf (outlog, "Creating %s\n", dupename);
		f=fopen(dupename, "a");
		if (f) {
		    fclose (f);
		} else {
		    fprintf (outlog, "Can't create %s\n", dupename);
		}
	    }
	    nfree(dupename);
	}
    }

    // deinit SMAPI
    MsgCloseApi();

    disposeConfig(config);
    fprintf(outlog,"Done\n");
    return (0);
}
