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
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#include <smapi/compiler.h>

#ifdef HAS_PROCESS_H
#  include <process.h>
#endif

#ifdef HAS_UNISTD_H
#  include <unistd.h>
#  include <strings.h>
#endif

#ifdef HAS_IO_H
#  include <io.h>
#endif

#ifdef HAS_SHARE_H
#  include <share.h>
#endif

#ifdef HAS_DOS_H
#  include <dos.h>
#endif

#include <smapi/msgapi.h>
#include <smapi/prog.h>
#include <smapi/progprot.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <fidoconf/xstr.h>
#include <fidoconf/log.h>
#include <fidoconf/afixcmd.h>


#include "cvsdate.h"

s_fidoconfig *config;

FILE *outlog;
char *version = "1.11";

typedef enum senduns { eNobody, eFirstLink, eNodes, eAll} e_senduns;

e_senduns sendUnSubscribe = eNodes;
int     delFromConfig = 0;
int     eraseBase = 1;
int     killPass = 0;
int     createDupe = 0;
static  time_t  tnow;
const   long    secInDay = 3600*24;


void usage(void) {

    fprintf(outlog, "hptkill %s %s\n", version, cvs_date);
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

int changeconfig(char *fileName, s_area *area) {
    char *cfgline=NULL, *token=NULL, *tmpPtr=NULL, *line=NULL;
    long strbeg = 0, strend = -1;

    char *areaName = area->areaName;

    w_log(LL_FUNC, __FILE__ "::changeconfig(%s,...)", fileName);

    if (init_conf(fileName))
		return -1;

    while ((cfgline = configline()) != NULL) {
        line = sstrdup(cfgline);
        line = trimLine(line);
        line = stripComment(line);
        if (line[0] != 0) {
            line = shell_expand(line);
            line = tmpPtr = vars_expand(line);
            token = strseparate(&tmpPtr, " \t");
            if (stricmp(token, "echoarea")==0) {
                token = strseparate(&tmpPtr, " \t");
                if (*token=='\"' && token[strlen(token)-1]=='\"' && token[1]) {
                    token++;
                    token[strlen(token)-1]='\0';
                }
                if (stricmp(token, areaName)==0) {
                    fileName = sstrdup(getCurConfName());
                    strend = get_hcfgPos();
                    if(strbeg > strend) strbeg = 0;
                    break;
                }
            }
        }
        strbeg = get_hcfgPos();
        nfree(line);
        nfree(cfgline);
    }
    close_conf();
    nfree(line);
    if (strend == -1) { /*  impossible   error occurred */
        nfree(cfgline);
        nfree(fileName);
        return -1;
    }
    nfree(cfgline);
    InsertCfgLine(fileName, cfgline, strbeg, strend);
    nfree(fileName);
    return 0;
}



int putMsgInArea(s_area *echo, s_message *msg)
{
    char *ctrlBuff, *textStart, *textWithoutArea;
    UINT textLength;
    HAREA harea;
    HMSG  hmsg;
    XMSG  msgHeader;
    int rc = 0;

    harea = MsgOpenArea((UCHAR *) echo->fileName, MSGAREA_CRIFNEC, (word)(echo->msgbType));
    if (harea != NULL) {
	hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);
	if (hmsg != NULL) {

	    textWithoutArea = msg->text;
	    textLength = strlen(textWithoutArea);

	    ctrlBuff = (char *) CopyToControlBuf((UCHAR *) textWithoutArea,
						 (UCHAR **) &textStart,
						 &textLength);

        msgHeader = createXMSG(config,msg, NULL, MSGLOCAL ,NULL);

	    MsgWriteMsg(hmsg, 0, &msgHeader, (UCHAR*)textStart, (dword)textLength, (dword)textLength, (dword)strlen(ctrlBuff), (byte *)ctrlBuff);

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
    s_message *msg;

    if (link->msg == NULL) 
    {
        msg = makeMessage(link->ourAka, &(link->hisAka), config->sysop,
            link->RemoteRobotName ? link->RemoteRobotName : "areafix",
            link->areaFixPwd ? link->areaFixPwd : "\x00", 1,
            config->areafixReportsAttr);
        msg->text = createKludges(config, NULL, link->ourAka, &(link->hisAka),
            "hptkill");
        if (config->areafixReportsFlags)
            xstrscat(&(msg->text), "\001FLAGS ", config->areafixReportsFlags, "\r",NULL);
        link->msg = msg;
    } else {
        msg = link->msg;
    }
    xscatprintf(&(msg->text), "-%s\r", areatag);
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

void update_queue(s_area *area)
{
    char *line    = NULL;
    FILE *queryFile;
    char seps[]   = " \t\n";
    char upDate   = 0;

    if(!config->areafixQueueFile)
        return;

    if ( !(queryFile = fopen(config->areafixQueueFile,"a+b")) ) /* can't open query file */
    {
       w_log(LL_ERR, "Can't open areafixQueueFile %s: %s", config->areafixQueueFile, strerror(errno) );
       return;
    }
    rewind(queryFile);
    while ((line = readLine(queryFile)) != NULL)
    {
        char* token = strtok( line, seps );
        if(strcasecmp(token,area->areaName) == 0)
        {
            upDate = 1;
            break;
        }
        nfree(line);
    }
    nfree(line)
    
    if (upDate == 0)
    {
        long eTime;
        struct  tm t1,t2;
        eTime = tnow + config->killedRequestTimeout*secInDay;
        t1 = *localtime( &tnow );
        t2 = *localtime( &eTime );
        xscatprintf( &line , "%s %s %d-%02d-%02d@%02d:%02d\t%d-%02d-%02d@%02d:%02d" ,
                area->areaName,
                "kill",
                t1.tm_year + 1900,
                t1.tm_mon  + 1,
                t1.tm_mday,
                t1.tm_hour,
                t1.tm_min,
                t2.tm_year + 1900,
                t2.tm_mon  + 1,
                t2.tm_mday,
                t2.tm_hour,
                t2.tm_min   );

        xstrscat(&line, " ", aka2str(*(area->useAka)), "\n", NULL);
        fputs(line , queryFile);
    }
    fclose(queryFile);
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

    update_queue(area);

	fprintf(outlog, "   deleting from config");
	if (changeconfig (getConfigFileName(),  area) != 0)
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
    fprintf(outlog, "done\n"); 
}


int main(int argc, char **argv) {
    
    int i, j;
    UINT k;
    struct _minf m;
    char **areas = NULL;
    char *needfree = NULL;
    char *line = NULL;
    int nareas=0;
    int found = 0;
    FILE *f = NULL;
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
        /* AreaName(s) specified by args */
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

    time( &tnow );

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
                            delArea = 2; /* if two links w/o pause - leave untouched */
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
                        delArea = 2; /* if two links w/o pause - leave untouched */
                        for (k=0; k < area->downlinkCount && delArea; k++) {
                            if (area->downlinks[k]->link->Pause == 0) delArea--;
                        }
                    }
                }
                if (delArea) {
                    delete_area(area);
                    killed++;
                    found++;
                    if (delFromConfig) { /* Area is removed from areas array! */
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
                    if (delFromConfig) { /* Area is removed from areas array! */
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
    /* Put mail for links to netmail */
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
            
            putMsgInArea(&(config->netMailAreas[0]), config->links[i].msg);
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
    
    /* deinit SMAPI */
    MsgCloseApi();
    
    disposeConfig(config);
    fprintf(outlog,"Done\n");
    return (0);
}
