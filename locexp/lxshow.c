/**********************************************************************
*   Copyright (C) 1999-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
***********************************************************************/

/* Routines that show specific data types */

#include "locexp.h"

#include "unicode/uset.h"
#include "unicode/ucurr.h"

/* Move Along.. nothing to see here.. */ 
U_CAPI UResourceBundle* U_EXPORT2 
ures_getByKeyWithFallback(const UResourceBundle *resB, 
                          const char* inKey, 
                          UResourceBundle *fillIn, 
                          UErrorCode *status);

/* Show a resource that's a collation rule list -----------------------------------------------------*/
/**
 * Show a string.  Make it progressive disclosure if it exceeds some length !
 * @param rb the resourcebundle to pull junk out of 
 * @param locale the name of the locale (for URL generation)
 * @param key the key we're listing
 */
void showCollationElements( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key )
{
  
    UErrorCode status = U_ZERO_ERROR;
    UErrorCode subStatus = U_ZERO_ERROR;
    const UChar *s  = 0;
    UChar temp[UCA_LEN]={'\0'};
    UChar *scopy = 0;
    UChar *comps = 0;
    UChar *compsBuf = 0;
    UBool bigString     = FALSE; /* is it too big to show automatically? */
    UBool userRequested = FALSE; /* Did the user request this string? */
    int32_t len = 0, len2, i;
    UCollator *coll = NULL; /* build an actual collator */
    UResourceBundle *array = NULL, *item = NULL;

    array = ures_getByKey(rb, key, array, &status);
    subStatus = status;
  
    s = ures_getStringByKey(array, "Sequence", &len, &subStatus);

    if(!s || !s[0] || (status == U_MISSING_RESOURCE_ERROR) || U_FAILURE(subStatus) )
    {

        /* Special case this to fetch what REALLY happens in this case ! */
        status = U_ZERO_ERROR;
    
        coll = ucol_open(locale, &status);

        if(U_SUCCESS(status))
        {
            if(strcmp(locale,"root")==0)
            {
                len = ucol_getRulesEx(coll,UCOL_FULL_RULES, temp,UCA_LEN);
                s=temp;
            }
            else
            {
                s = ucol_getRules(coll,&len);
            }        

        }
    
        if( (s==0) || (*s == 0) )
        {
            /* FALLBACK - load from root */
            len = ucol_getRulesEx(coll,UCOL_FULL_RULES, temp,UCA_LEN);
            s=temp;
            status = U_USING_DEFAULT_WARNING;
        }
    }
    else
    {
        len = u_strlen(s);
    }

    len2 = len;

    scopy = malloc(len * sizeof(UChar));
    memcpy(scopy, s, len*sizeof(UChar));

    for(i=0;i<len;i++)
    {
        if(scopy[i] == 0x0000)
        {
            scopy[i] = 0x0020; /* normalizer seems to croak on nulls */
        }

    }
    s = scopy;

    if(U_SUCCESS(status) && ( len > kShowStringCutoffSize ) )
    {
        bigString = TRUE;
        userRequested = didUserAskForKey(lx, key);
    }

    showKeyAndStartItemShort(lx, key, NULL, locale, FALSE, status);

    u_fprintf(lx->OUT, "&nbsp;</TD></TR><TR><TD></TD><TD>");
  
    showExploreButtonSort(lx, rb,locale,  "CollationElements", TRUE);

    u_fprintf(lx->OUT, "</TD>"); /* Now, we're done with the ShowKey.. cell */

    u_fprintf(lx->OUT, "</TR><TR><TD COLSPAN=2>");

    if(U_SUCCESS(status))
    {

        if(bigString && !userRequested) /* it's hidden. */
        {
            u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
        }
        else
        {
            if(bigString)
            {
                u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                    locale,
                    key,
                    FSWF("bigStringHide", "Hide"));
            }

            if(U_SUCCESS(status))
            {

                compsBuf = malloc(sizeof(UChar) * (len*3));
                comps = compsBuf;
              
                {
                    for(i=0;i<(len*3);i++)
                    {
                        comps[i] = 0x0610;
                    }
                }

                len = unorm_normalize(s,
                                      len,
                                      UNORM_NFKC,
                                      0,
                                      comps,
                                      len*3,
                                      &status);

              
/*              u_fprintf(lx->OUT, "xlit: %d to %d<P>\n",
                len2,len); */
                if(U_FAILURE(status))
                {
                    free(compsBuf);
                    u_fprintf(lx->OUT, "xlit failed -} %s<P>\n",
                              u_errorName(status));
                    comps = (UChar*)s;
                    compsBuf = comps;
                    len = len2;
                }

                /* DO NOT collect chars from the normalization rules.
                   Sorry, but they contain decomposed chars which mess up the codepage selection.. */
/*
  oldCallback = ucnv_getFromUCallBack((UConverter*)u_fgetConverter(lx->OUT));
  if(oldCallback != UCNV_FROM_U_CALLBACK_TRANSLITERATED)
  {
  ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), COLLECT_lastResortCallback, &status);
  }
*/
                if(*comps == 0)
                {
                    u_fprintf(lx->OUT, "<I>%S</I>", FSWF("empty", "(Empty)"));
                }
                else while(len--)
                {

                    if(*comps == '&')
                    {
                        u_fprintf(lx->OUT, "<P>&amp;");
                    }
                    else if(*comps == '<')
                    {
                        if((comps != compsBuf) && (comps[-1] != '<'))
                        {
                            /* don't break more than once. */
                            u_fprintf(lx->OUT, "<BR>&nbsp;");
                        }
                        u_fprintf(lx->OUT, "&lt;");
                    } else if(*comps == ']')
                    {
                        u_fprintf(lx->OUT, "]<p>\r\n");
                    }
                    else
                    {
                        if((*comps == 0x000A) || u_isprint(*comps))
                            u_fprintf(lx->OUT, "%C", *comps);
                        else
                            u_fprintf(lx->OUT, "<B>\\u%04X</B>", *comps); /* to emulate the callback */
                    }

                    comps++;
                };

/* **	      ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), oldCallback, &status); */

	    }
            else
                explainStatus(lx, status, key);
	}
    }

    status = U_ZERO_ERROR;
    s = ures_getStringByKey(array, "Version", &len, &status);
    if(U_SUCCESS(status))
    {
        u_fprintf(lx->OUT, "<P><B>%S %S:</B> %S\r\n",
                  FSWF("Collation", "Collation"),
                  FSWF("Version","Version"),s);
    }

    u_fprintf(lx->OUT, "</TD>");
  

    free(scopy);
    if(coll) ucol_close(coll);
  
    showKeyAndEndItem(lx, key, locale);
    ures_close(array);
    ures_close(item);
}

/* These aren't resources anymore. */
void showLocaleCodes(LXContext *lx,  UResourceBundle *rb, const char *locale)
{
  
    UErrorCode status = U_ZERO_ERROR;
    char tempctry[1000], templang[1000], tempvar[1000];
    const char *ctry3 = NULL, *lang3 = NULL;

    showKeyAndStartItem(lx, "LocaleCodes", FSWF("LocaleCodes", "Locale Codes"), locale, FALSE, status);

    u_fprintf(lx->OUT, "<table summary=\"%S\" border=1><tr><td></td><td><b>%S</b></td><td><b>%S</b></td><td><b>%S</b></td></tr>\r\n",
              FSWF("LocaleCodes", "Locale Codes"),
              FSWF("LocaleCodes_Language", "Language"),
              FSWF("LocaleCodes_Country", "Region"),
              FSWF("LocaleCodes_Variant", "Variant"));
    u_fprintf(lx->OUT, "<tr><td></td><td>");
  
    status = U_ZERO_ERROR;
    uloc_getLanguage(locale, templang, 1000, &status);
    if(U_SUCCESS(status))
    {
        u_fprintf(lx->OUT, templang);
    }
    else
    {
        explainStatus(lx, status, "LocaleCodes");
        templang[0] = 0;
    }
  
    u_fprintf(lx->OUT, "</td><td>");
  
    status = U_ZERO_ERROR;
    uloc_getCountry(locale, tempctry, 1000, &status);
    if(U_SUCCESS(status))
    {
        u_fprintf(lx->OUT, tempctry);
    }
    else
    {
        explainStatus(lx, status, "LocaleCodes");
        tempctry[0] = 0;
    }
  
    u_fprintf(lx->OUT, "</td><td>");
  
    status = U_ZERO_ERROR;
    uloc_getVariant(locale, tempvar, 1000, &status);
    if(U_SUCCESS(status))
        u_fprintf(lx->OUT, tempvar);
    else
        explainStatus(lx, status, "LocaleCodes");

    u_fprintf(lx->OUT, "</TD></TR>\r\n");

    /* 3 letter line */

    u_fprintf(lx->OUT, "<tr><td>%S</td>",
              FSWF("LocaleCodes_3", "3"));

    u_fprintf(lx->OUT, "<td>");

    lang3 = uloc_getISO3Language(locale);
    if(lang3)
    {
        u_fprintf(lx->OUT, "%s", lang3);
    }

    u_fprintf(lx->OUT, "</td><td>");

    ctry3 = uloc_getISO3Country(locale);
    if(ctry3)
    {
        u_fprintf(lx->OUT, "%s", ctry3);
    }

    u_fprintf(lx->OUT, "</td><td></td></tr>\r\n");
  
    u_fprintf(lx->OUT, "</table>\r\n");

    u_fprintf(lx->OUT, "</td>");
    showKeyAndEndItem(lx, "LocaleCodes", locale);  /* End of LocaleCode's sub item */

}

/* -------------- show script for locale --------------*/
void showLocaleScript(LXContext *lx, UResourceBundle *rb, const char *locale)
{
  
    UErrorCode status = U_ZERO_ERROR;

    UScriptCode  list[32];
    int32_t len, i;

    len = uscript_getCode(locale, list, sizeof(list)/sizeof(list[0]), &status);

    showKeyAndStartItem(lx, "LocaleScript", FSWF("LocaleScript", "Locale Script"), locale, FALSE, status);

    u_fprintf(lx->OUT, "<table summary=\"%S\" border=1>\r\n",
              FSWF("LocaleScript", "Locale Script"));
    u_fprintf(lx->OUT, "<tr><td><b>%S</b></td><td><b>%S</b></td></tr>\r\n",
              FSWF("LocaleScriptAbbreviations", "Short Names"),
              FSWF("LocaleScriptNames", "Long Names")
        );
  
    for(i=0;i<len;i++)
    {
        u_fprintf(lx->OUT, "   <tr><td>%s</td><td>%s</td>\r\n", 
                  uscript_getShortName(list[i]), uscript_getName(list[i]));
    }
    u_fprintf(lx->OUT, "</tr>");
    u_fprintf(lx->OUT, "</table>\r\n");
    u_fprintf(lx->OUT, "</td>\r\n");

    showKeyAndEndItem(lx, "LocaleScript", locale);
}

/* Show a resource that's a simple integer -----------------------------------------------------*/
/**
 * Show an integer
 * @param rb the resourcebundle to pull junk out of 
 * @param locale the name of the locale (for URL generation)
 * @param radix base of number to display (Only 10 and 16 are supported)
 * @param key the key we're listing
 */

void showInteger( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key, int radix)
{
  
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *res = NULL;
    int32_t i;

    res = ures_getByKey(rb, key, res, &status);
    i = ures_getInt(res, &status);
    showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);

    if(U_SUCCESS(status))
    {
        if(radix == 10) {
            u_fprintf(lx->OUT, "%d", i);
        } else if(radix == 16) {
            u_fprintf(lx->OUT, "0x%X", i);
        } else {
            u_fprintf(lx->OUT, "(Unknown radix %d for %d)", radix, i);
        }
    }
    u_fprintf(lx->OUT, "</TD>");
    showKeyAndEndItem(lx, key, locale);
}

/* Show a resource that's a simple string -----------------------------------------------------*/
/**
 * Show a string.  Make it progressive disclosure if it exceeds some length !
 * @param rb the resourcebundle to pull junk out of 
 * @param locale the name of the locale (for URL generation)
 * @param key the key we're listing
 */

void showString( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key, UBool PRE )
{
  
    UErrorCode status = U_ZERO_ERROR;
    const UChar *s  = 0;
    UBool bigString = FALSE; /* is it big? */
    UBool userRequested = FALSE; /* Did the user request this string? */
    int32_t len;

    s = ures_getStringByKey(rb, key, &len, &status);

    if(U_SUCCESS(status) && ( u_strlen(s) > kShowStringCutoffSize ) )
    {
        bigString = TRUE;
        userRequested = didUserAskForKey(lx, key);
    }

    showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);

    if(U_SUCCESS(status))
    {

        if(bigString && !userRequested) /* it's hidden. */
        {
            u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
        }
        else
        {
            if(bigString)
            {
                u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                    locale,
                    key,
                    FSWF("bigStringHide", "Hide"));
            }

            if(U_SUCCESS(status))
            {
                if(PRE)
                    u_fprintf(lx->OUT, "<PRE>");

                if(*s == 0)
                    u_fprintf(lx->OUT, "<I>%S</I>", FSWF("empty", "(Empty)"));
                {
                    writeEscaped(lx, s);
                }

                if(PRE)
                    u_fprintf(lx->OUT, "</PRE>");
            }
        }
    }
    u_fprintf(lx->OUT, "</TD>");
    showKeyAndEndItem(lx, key, locale);
}

/* Show a resource that's a UnicodeSet -----------------------------------------------------*/
/**
 * Show a string.  Make it progressive disclosure if it exceeds some length !
 * @param rb the resourcebundle to pull junk out of 
 * @param locale the name of the locale (for URL generation)
 * @param key the key we're listing
 */

void showUnicodeSet( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key, UBool PRE )
{
  
    UErrorCode status = U_ZERO_ERROR;
    const UChar *s  = 0;
    UChar smallString[ kShowUnicodeSetCutoffSize + 1];
    UBool bigString = FALSE; /* is it big? */
    UBool userRequested = FALSE; /* Did the user request this string? */
    int32_t setLen = 0, rulesLen = 0, len;
    int32_t i;
    USet *uset;
    UChar *buf  = NULL;
    int32_t bufSize = 0;
    int32_t howManyChars = 0;
    
    bufSize = 512;
    buf = (UChar*)malloc(bufSize * sizeof(UChar));
  
    s = ures_getStringByKey(rb, key, &rulesLen, &status);

#if defined (LX_UBROWSE_PATH)
    u_fprintf(lx->OUT, "<FORM METHOD=GET ACTION=\"%s\">\n", LX_UBROWSE_PATH);
    u_fprintf(lx->OUT, "<INPUT TYPE=HIDDEN NAME=GO><INPUT TYPE=hidden NAME=us VALUE=\"%S\"><input type=hidden name=gosetn value=\"\">\n", s);
    u_fprintf(lx->OUT, "<INPUT TYPE=IMAGE WIDTH=48 HEIGHT=20 BORDER=0 SRC=\"" LDATA_PATH "explore.gif\"  ALIGN=RIGHT   ");
    u_fprintf(lx->OUT, " VALUE=\"%S\"></FORM>",
              FSWF("exploreTitle", "Explore"));
    u_fprintf(lx->OUT, "</FORM>");
#endif
    
    uset = uset_openPattern(s, rulesLen, &status);

    showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);

    if(U_FAILURE(status))
    {
        u_fprintf(lx->OUT, "</TD>");
        showKeyAndEndItem(lx, key, locale);
        return;
    }

    setLen = uset_size(uset);

    if( (rulesLen > kShowUnicodeSetCutoffSize ) ||
        (setLen > kShowUnicodeSetCutoffSize) )
    {
        userRequested = didUserAskForKey(lx, key);
        bigString = TRUE;

        if(userRequested) /* it's not hidden. */
        {
            u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                      locale,
                      key,
                      FSWF("bigStringShorten", "Don't show all"));
        }
    }

    /** RULES **/
    u_fprintf(lx->OUT, "<table border=1 summary=\"Rules\" cellpadding=3 cellspacing=3><tr><td><b>%S</b></td><td>\r\n",
              FSWF("rules", "Rules"));

    /* Always do this loop */

    if( (rulesLen > kShowUnicodeSetCutoffSize) && !userRequested ) 
    { 
        /* shorten string */
        u_strncpy(smallString, s, kShowUnicodeSetCutoffSize);
        smallString[kShowUnicodeSetCutoffSize] = 0;
        s = smallString;
        len = kShowUnicodeSetCutoffSize;
    }
    else
    {
        len = rulesLen;
    }
  
    u_fprintf_u(lx->OUT, s);

    if(len != rulesLen)
    {
        u_fprintf(lx->OUT, "%S", FSWF("...", "..."));
        u_fprintf(lx->OUT, "<br><A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\"><I>%S</I> ", locale, key,key, FSWF("bigStringClickToExpand","Truncated due to size. Click here to show. "));
        u_fprintf_u(lx->OUT, FSWF("bigStringSize", "(%d of %d shown)"), len, rulesLen);
        u_fprintf(lx->OUT, " </A>\r\n");
    }
  
    u_fprintf(lx->OUT, "</td></tr>\r\n");

    /** Set **/

    u_fprintf(lx->OUT, "<tr><td><b>%S</b></td><td>\r\n",
              FSWF("set", "Set"));
  
    for(i=0;i<uset_getItemCount(uset);i++)
    {
        UErrorCode subErr = U_ZERO_ERROR;
        UChar32 start;
        UChar32 end;
        int32_t n;
        
        n = uset_getItem(uset, i, &start,&end, buf, bufSize, &subErr);
        if(subErr == U_BUFFER_OVERFLOW_ERROR)
        {
            free(buf);
            bufSize = (n+4);
            buf = (UChar*)realloc(buf, sizeof(UChar)*bufSize);
            subErr = U_ZERO_ERROR;
            n = uset_getItem(uset, i, &start,&end, buf, bufSize, &subErr);
        }
        if(U_FAILURE(subErr))
        {
            u_fprintf(lx->OUT, "<B>Fatal: err in showSet, %s @ index %d\n", u_errorName(subErr), i);
            u_fprintf(lx->OUT, "</td></tr></table><HR>\r\n");
            return;
        }
        if(n == 0) /* range */
        {
            int32_t thisRangeLen;
            UChar32 c;

            thisRangeLen =  (end-start)+1;

            if(!userRequested)
            { 
                
                if((howManyChars < kShowUnicodeSetCutoffSize) &&
                   (howManyChars+(end-start)>kShowUnicodeSetCutoffSize))
                {
                    /* truncate */
                    end = start+(kShowUnicodeSetCutoffSize-howManyChars);
                }
            }

            if(userRequested || (howManyChars <= kShowUnicodeSetCutoffSize))
            {
                for(c = start; c<=end; c++)
                {
                    u_fprintf(lx->OUT, "%C", c);
                }
            }
            howManyChars += thisRangeLen;
        }
        else
        {
            if(i != 0)
            {
                u_fprintf(lx->OUT, ", ");
            }
            buf[n]=0;
            u_fprintf(lx->OUT, "\"%S\"", buf);
        }
    }

    if(!userRequested && (howManyChars >= kShowUnicodeSetCutoffSize))
    {
        u_fprintf(lx->OUT, "%S", FSWF("...", "..."));
        u_fprintf(lx->OUT, "<br><A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\"><I>%S</I> ", locale, key,key, FSWF("bigStringClickToExpand","Truncated due to size. Click here to show. "));
        u_fprintf_u(lx->OUT, FSWF("bigStringSize", "(%d of %d shown)"), kShowUnicodeSetCutoffSize, howManyChars);
        u_fprintf(lx->OUT, " </A>\r\n");
    }
  
    u_fprintf(lx->OUT, "</td></tr></table>\r\n");

    u_fprintf(lx->OUT, "</TD>");
    showKeyAndEndItem(lx, key, locale);
    free(buf);
    uset_close(uset);
}

/* Show a resource that's a simple string, but explain each character.-----------------------------------------------------*/
/**
 * Show a string.  Make it progressive disclosure if it exceeds some length !
 * @param rb the resourcebundle to pull junk out of 
 * @param locale the name of the locale (for URL generation)
 * @param desc array (0 at last item) of char desc
 * @param key the key we're listing
 */

void showStringWithDescription( LXContext *lx, UResourceBundle *rb, const char *locale, const UChar *desc[], const char *key, UBool hidable)
{
  
    UErrorCode status = U_ZERO_ERROR;
    const UChar *s  = 0;
    UBool bigString = FALSE; /* is it big? */
    UBool userRequested = FALSE; /* Did the user request this string? */
    int32_t i;
    int32_t len;

    s = ures_getStringByKey(rb, key, &len, &status);

    /* we'll assume it's always big, for now. */
    bigString = TRUE;
    userRequested = didUserAskForKey(lx, key);

    showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);

    /** DON'T show the string as a string. */
    /* 
       if(U_SUCCESS(status) && s)
       u_fprintf(lx->OUT, "%S<BR>\r\n", s);
    */
    if(!hidable)
    {
        userRequested = TRUE;
        bigString = FALSE;
    }
  

    if(bigString && !userRequested) /* it's hidden. */
    {
        u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("stringClickToShow","(Click here to show.)"));
        u_fprintf(lx->OUT, "<P>");
    }
    else
    {
        if(bigString)
        {
            u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                locale,
                key,
                FSWF("bigStringHide", "Hide"));
        }
  
        if(U_SUCCESS(status))
        {
            u_fprintf(lx->OUT, "<table summary=\"String\" BORDER=1 WIDTH=100%%>");
            u_fprintf(lx->OUT, "<tr><td><b>%S</b></td><td><b>%S</b></td><td><b>%S</b></td></tr>\r\n",
                FSWF("charNum", "#"),
                FSWF("char", "Char"),
                FSWF("charMeaning", "Meaning"));


            for(i=0;desc[i];i++)
            {
                if(!s[i])
                    break;

                u_fprintf(lx->OUT, "<TR><TD WIDTH=5>%d</TD><TD>%C</TD><TD>%S</TD></TR>\r\n",
                    i,
                    s[i],
                    desc[i]);
            }
            u_fprintf(lx->OUT, "</table>\r\n");
        }
    }
    u_fprintf(lx->OUT, "</TD>");
    showKeyAndEndItem(lx, key, locale);
}
  
/* Show a resource that's an array. Useful for types we haven't written viewers for yet --------*/

void showArray( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key, ECal isCal )
{
    UErrorCode status = U_ZERO_ERROR;
    UErrorCode firstStatus = U_ZERO_ERROR;
    UResourceBundle  *array = NULL, *item = NULL;
    int32_t len;
    const UChar *s  = 0;
    int i;
    const char *realKey;
    char   key2[1024];
    UBool userRequested = FALSE;
    UBool isDefault = FALSE;

    userRequested = didUserAskForKey(lx, key);
    strcpy(key2, key);
    if(isCal == kCal) {
      key2[0]=toupper(key2[0]); /* upcase for compatibility */
      array = loadCalRes(lx, key, &isDefault, &firstStatus);
    }

    if(array == NULL ) {
      array = ures_getByKey(rb, key, array, &firstStatus);
    }

    item = ures_getByIndex(array, 0, item, &firstStatus);

    showKeyAndStartItem(lx, key2, NULL, locale, FALSE, firstStatus);

    if(realKey == key2) {
      u_fprintf(lx->OUT, "(%s)<br>\r\n", lx->defaultCalendar);
    }

    u_fprintf(lx->OUT, "<OL>\r\n");

    for(i=0;;i++)
    {
        status = U_ZERO_ERROR;
        if(U_FAILURE(firstStatus)) {
            status = firstStatus; /* ** todo: clean up err handling! */
        }

        if((i > 10) && !userRequested) {
          u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
          break;
        }

        item = ures_getByIndex(array, i, item, &status);
        s  = ures_getString(item, &len, &status);

        if(!s)
            break;

        if(U_SUCCESS(status))
            u_fprintf(lx->OUT, "<LI> %S\r\n", s);
        else
        {
            u_fprintf(lx->OUT, "<LI>");
            explainStatus(lx, status, key);
            u_fprintf(lx->OUT, "\r\n");
            break;
        }

    }
    u_fprintf(lx->OUT, "</OL>");
    if((i>=10) && userRequested) {
      u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                locale,
                key,
                FSWF("bigStringHide", "Hide"));
    }
    if(isDefault) {
      calPrintDefaultWarning(lx);
    }
    u_fprintf(lx->OUT, "</TD>");
    u_fprintf(lx->OUT, "<!-- %s:%d -->\r\n", __FILE__, __LINE__);
    showKeyAndEndItem(lx, key, locale);
    u_fprintf(lx->OUT, "<!-- %s:%d -->\r\n", __FILE__, __LINE__);

    if(U_SUCCESS(status)) {
      ures_close(item);
      ures_close(array);
    }
}

/* Show a resource that's an array, wiht an explanation ------------------------------- */

void showArrayWithDescription( LXContext *lx, UResourceBundle *rb, const char *locale, const UChar *desc[], const char *key, ECal isCal )
{
    UErrorCode status = U_ZERO_ERROR;
    UErrorCode firstStatus;
    const UChar *s  = 0;
    UChar *toShow =0;
    UChar nothing[] = {(UChar)0x00B3, (UChar)0x0000};
    UResourceBundle  *array = NULL, *item = NULL;
    int32_t len;

    enum { kNoExample = 0, kDateTimeExample, kNumberExample } exampleType;
    int32_t i;
    UDate now;  /* example date */
    double d = 1234.567;   /* example number */
    UDateFormat   *exampleDF = 0;
    UNumberFormat *exampleNF = 0;
    UErrorCode exampleStatus = U_ZERO_ERROR;
    UChar tempChars[1024];
    UChar tempDate[1024]; /* for Date-Time */
    UChar tempTime[1024]; /* for Date-Time */
    /*const char *realKey;
      char   key2[1024];*/
    UBool isDefault = FALSE;
    /* figure out what example to use */
    if(!strcmp(key,"DateTimePatterns"))
        exampleType = kDateTimeExample;
    else if(!strcmp(key, "NumberPatterns"))
        exampleType = kNumberExample;
    else
        exampleType = kNoExample;

    tempDate[0]=0;
    tempTime[0]=0;

    /* store the date now..just in case formatting takes multiple seconds! */
    now = ucal_getNow();

    firstStatus = U_ZERO_ERROR;
    
    if( isCal == kCal) {
      array = loadCalRes(lx, key, &isDefault, &firstStatus);
    }
    
    if(array == NULL ) {
      array = ures_getByKey(rb, key, array, &firstStatus);
    }

    item = ures_getByIndex(array, 0, item, &firstStatus);
    s = ures_getString(item, &len, &firstStatus);
    showKeyAndStartItemShort(lx, key, NULL, locale, FALSE, firstStatus);

    #if 0
    if(realKey == key2) {
      u_fprintf(lx->OUT, "(%s)<br>\r\n", lx->defaultCalendar);
    }
    #endif

    if(exampleType != kNoExample)
    {
        toShow = nothing+1;

        exampleStatus = U_ZERO_ERROR;

        switch(exampleType)
        {

        case kDateTimeExample:
            exampleStatus = U_ZERO_ERROR;
            exampleDF = udat_open(UDAT_IGNORE,UDAT_IGNORE,locale,NULL, 0, s,-1,&exampleStatus);
            if(U_SUCCESS(exampleStatus))
            {
                len = udat_toPattern(exampleDF, TRUE, tempChars, 1024,&exampleStatus);
                if(U_SUCCESS(exampleStatus))
                {
                    toShow = tempChars;
                }
                unum_close(exampleDF);
                exampleDF = NULL;
            }
            break;

        case kNumberExample:

            toShow = nothing;

            exampleNF = unum_open(0, s,-1,locale,NULL, &exampleStatus);
            if(U_SUCCESS(exampleStatus))
            {
                len = unum_toPattern(exampleNF, TRUE, tempChars, 1024, &exampleStatus);
                if(U_SUCCESS(exampleStatus))
                {
                    toShow = tempChars;
                }
                unum_close(exampleNF);
                exampleNF = NULL;
            }
            break;
        default:
            ;
        }
        exampleStatus = U_ZERO_ERROR;
        showExploreButton(lx, rb, locale, toShow, key);
    }
    else
    {
        u_fprintf(lx->OUT, "&nbsp;");
    }

#ifdef LX_USE_CURRENCY
    /* Currency Converter link */
    if(!strcmp(key, "CurrencyElements"))
    {
        UErrorCode curStatus = U_ZERO_ERROR;
        UChar *curStr = NULL, *homeStr = NULL;

        /* index [1] is the int'l currency symbol */
        item = ures_getByIndex(array, 1, item, &curStatus);
        curStr  = ures_getString(item, &len, &curStatus);
        if(lx->defaultRB)
        {
            item = ures_getByKey(lx->defaultRB, key, item, &curStatus);
            item = ures_getByIndex(item, 1, item, &curStatus);
            curStr  = ures_getString(item, &len, &curStatus);

            /* homeStr = ures_getArrayItem(lx->defaultRB, key, 1, &curStatus); */
        }
        else
            homeStr = (const UChar[]){0x0000};
      
        /* OANDA doesn't quite follow the same conventions for currency.  

           TODO:

           RUR->RUB
           ...
        */

      
        u_fprintf(lx->OUT, "<FORM TARGET=\"_currency\" METHOD=\"POST\" ACTION=\"http:www.oanda.com/converter/travel\" ENCTYPE=\"x-www-form-encoded\"><INPUT TYPE=\"hidden\" NAME=\"result\" VALUE=\"1\"><INPUT TYPE=\"hidden\" NAME=\"lang\" VALUE=\"%s\"><INPUT TYPE=\"hidden\" NAME=\"date_fmt\" VALUE=\"us\"><INPUT NAME=\"exch\" TYPE=HIDDEN VALUE=\"%S\"><INPUT TYPE=HIDDEN NAME=\"expr\" VALUE=\"%S\">",
                  "en", /* lx->dispLocale */
                  curStr,
                  homeStr
            );

        u_fprintf(lx->OUT, "<INPUT TYPE=IMAGE WIDTH=48 HEIGHT=20 BORDER=0 SRC=\"" LDATA_PATH "explore.gif\"  ALIGN=RIGHT   ");
        u_fprintf(lx->OUT, " VALUE=\"%S\"></FORM>",
                  FSWF("exploreTitle", "Explore"));
        u_fprintf(lx->OUT, "</FORM>");
    }
#endif
    u_fprintf(lx->OUT, "</td>"); /* Now, we're done with the ShowKey.. cell */

    u_fprintf(lx->OUT, "</TR><tr><td colspan=2><table border=2 width=\"100%%\">\r\n");

    for(i=0;desc[i];i++)
    {
      
        u_fprintf(lx->OUT, "<tr><td width=5>%d</td><td>%S</td><td>",
                  i, desc[i]);

        status = U_ZERO_ERROR;
        exampleStatus = U_ZERO_ERROR;

        item = ures_getByIndex(array, i, item, &status);
        s =    ures_getString(item, &len, &status);

        if(i==0)
            firstStatus = status;

        if(U_SUCCESS(status) && s)
        {
            toShow = (UChar*) s;

            switch(exampleType)
            {
            case kDateTimeExample: /* localize pattern.. */
                if(i < 8)
                {
                    len = 0;

                    exampleDF = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale,NULL, 0, s,-1,&exampleStatus);
                    if(U_SUCCESS(exampleStatus))
                    {
                        len = udat_toPattern(exampleDF, TRUE, tempChars, 1024,&exampleStatus);
                        
                        if(U_SUCCESS(exampleStatus))
                        {
                            toShow = tempChars;
                        }
                    }
                }
                break;
                
            case kNumberExample:
                if(i == 3) /* scientific */
                    d = 1234567890;

                exampleNF = unum_open(0, s,-1,locale, NULL, &exampleStatus);
                if(U_SUCCESS(exampleStatus))
                {
                    len = unum_toPattern(exampleNF, TRUE, tempChars, 1024, &exampleStatus);
                    if(U_SUCCESS(exampleStatus))
                    {
                        toShow = tempChars;
                    }
                }
                break;

            default:
                ;
            }

            u_fprintf(lx->OUT, "%S\r\n", toShow);
        }
        else
        {
            s = 0;
            explainStatus(lx, status, key);
            u_fprintf(lx->OUT, "\r\n");
            break;
        }
        u_fprintf(lx->OUT, "</td>");
      
        if(s) /* only if pattern exists */
            switch(exampleType)
            {
            case kDateTimeExample:
                if(i < 8)
                {
                    u_fprintf(lx->OUT, "<TD>");

                    if(U_SUCCESS(exampleStatus))
                    {
                        exampleStatus = U_ZERO_ERROR; /* clear fallback info from exampleDF */
                        udat_format(exampleDF, now, tempChars, 1024, NULL, &exampleStatus);
                        udat_close(exampleDF);

                        if(U_SUCCESS(exampleStatus))
                            u_fprintf(lx->OUT, "%S", tempChars);

                    }
                    explainStatus(lx, exampleStatus, key);
                    u_fprintf(lx->OUT, "</td>\r\n");
                    
                    if(U_SUCCESS(exampleStatus)) {
                      if(i == 3) /* short time */
                        u_strcpy(tempTime, tempChars);
                      else if(i == 7) /* short date */
                        u_strcpy(tempDate, tempChars);
                    }
                }
                else
                {
                    u_fprintf(lx->OUT, "<td>");
                    exampleStatus = U_ZERO_ERROR;
                    if(s) {
                        if(u_formatMessage(locale, s, -1, tempChars,1024,&exampleStatus, 
                                           tempTime,
                                           tempDate)) {
                          if(U_SUCCESS(exampleStatus)) {
                            u_fprintf(lx->OUT,"%S", tempChars);
                          } else {
                            explainStatus(lx, exampleStatus, key);
                          }
                        }
                    }
                    u_fprintf(lx->OUT, "</td>\r\n");
                }
                break;

            case kNumberExample:
            {
                u_fprintf(lx->OUT, "<td>");

                if(U_SUCCESS(exampleStatus))
                {
                    exampleStatus = U_ZERO_ERROR; /* clear fallback info from exampleDF */

                    if(i == 3) /* scientific */
                        d = 1234567890;
                    unum_formatDouble(exampleNF, d, tempChars, 1024, NULL, &exampleStatus);

                    if(U_SUCCESS(exampleStatus))
                        u_fprintf(lx->OUT, "%S", tempChars);


                    u_fprintf(lx->OUT, "</TD><TD>");

                    if(i == 3) /* scientific */
                        d = 0.00000000000000000005;

                    unum_formatDouble(exampleNF, -d, tempChars, 1024, NULL, &exampleStatus);

                    if(U_SUCCESS(exampleStatus))
                        u_fprintf(lx->OUT, "%S", tempChars);

                    unum_close(exampleNF);

                }
                explainStatus(lx, exampleStatus, key);
                u_fprintf(lx->OUT, "</td>\r\n");
            }
            break;

            case kNoExample:
            default:
                break;
            }

        u_fprintf(lx->OUT, "</tr>\r\n");

    }


    u_fprintf(lx->OUT, "</table>");

    if(isDefault) {
      calPrintDefaultWarning(lx);
    }
    u_fprintf(lx->OUT, "</td>");

    showKeyAndEndItem(lx, key, locale);
    ures_close(item);
    ures_close(array);
}

void showSpelloutExample( LXContext *lx, UResourceBundle *rb, const char *locale)
{
    UErrorCode status;
    double examples[] = { 0, 123.45, 67890 };
    UNumberFormat *exampleNF = 0;
    UNumberFormatStyle styles[] = { UNUM_SPELLOUT 
#if defined (UNUM_ORDINAL)
                                    , UNUM_ORDINAL, UNUM_DURATION
#endif
    };
    const char *stylen[] = { "spellout", "ordinal", "duration" }; 
    int n, k;
    const char *key = "SpelloutRulesExample";
    UChar tempChars[245];

    status = U_ZERO_ERROR;
    exampleNF = unum_open(UNUM_SPELLOUT,NULL, -1, locale, NULL, &status);

    showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);
    if(exampleNF) unum_close(exampleNF);

    u_fprintf(lx->OUT, "<table border=2 width=\"100%%\">\r\n");

    for(k=0;k<sizeof(styles)/sizeof(styles[0]);k++) {
      u_fprintf(lx->OUT, "<tr><td colspan=2><b>%s</b></td></tr>\r\n", stylen[k]);
      for(n=0;n<(sizeof(examples)/sizeof(examples[0]));n++) {
        status = U_ZERO_ERROR;
        tempChars[0] = 0;
        exampleNF = unum_open(styles[k],NULL, -1, locale, NULL, &status);
        unum_formatDouble(exampleNF, examples[n], tempChars, 1024,0, &status);
        u_fprintf(lx->OUT, "<tr><td>%f</td><td>%S", examples[n], tempChars);
        unum_close(exampleNF);
        if(U_FAILURE(status)) {
          u_fprintf(lx->OUT, " ");
          explainStatus(lx, status, NULL);
        }
        u_fprintf(lx->OUT, "</td></tr>\r\n");
      }
    }

    u_fprintf(lx->OUT, "</table>");
    showKeyAndEndItem(lx, key, locale);
}

/* show the DateTimeElements string *------------------------------------------------------*/

void showDateTimeElements( LXContext *lx, UResourceBundle *rb, const char *locale)
{
    UErrorCode status = U_ZERO_ERROR;
    const UChar *s  = 0;
    int32_t    len;
    const int32_t   *elements;

    UResourceBundle *array = NULL, *item = NULL;

    const char *key = "DateTimeElements";
    /*
      0: first day of the week 
      1: minimaldaysinfirstweek 
    */

    status = U_ZERO_ERROR;

    array = ures_getByKey(rb, key, array, &status);
    elements = ures_getIntVector(array, &len, &status);

    showKeyAndStartItem(lx, key, FSWF("DateTimeElements","Date and Time Options"), locale, FALSE, status);

    if(len < 2)
    {
        u_fprintf(lx->OUT, "%S ", FSWF("DateTimeElements_short", "Error- resource is too short (should be 2 elements)!"));
        ures_close(array);
        showKeyAndEndItem(lx, key, locale);
        return;
    }

    /* First day of the week ================= */
    u_fprintf(lx->OUT, "%S ", FSWF("DateTimeElements0", "First day of the week: "));

    if(U_SUCCESS(status))
    {
        int32_t  firstDayIndex;

        firstDayIndex = (((elements[0])+6)%7); 
      
        u_fprintf(lx->OUT, " %d \r\n", elements[0]);
        /* here's something fun: try to fetch that day from the user's current locale */
        status = U_ZERO_ERROR;
      
        if(lx->dispRB && U_SUCCESS(status))
        {
            /* don't use 'array' here because it's the DTE resource */
            item = ures_getByKey(lx->dispRB, "DayNames", item, &status);
            item = ures_getByIndex(item, firstDayIndex, item, &status);
            s    = ures_getString(item, &len, &status);
            
            if(s && U_SUCCESS(status))
            {
                u_fprintf(lx->OUT, " = %S \r\n", s);
            }
            status = U_ZERO_ERROR;

            item = ures_getByKey(rb, "DayNames", item, &status);
            item = ures_getByIndex(item, firstDayIndex, item, &status);
            s    = ures_getString(item, &len, &status);

            if(s && U_SUCCESS(status))
            {
                u_fprintf(lx->OUT, " = %S \r\n", s);
            }
        }
        status = U_ZERO_ERROR;
    }
    else
    {
        explainStatus(lx, status, key);
        u_fprintf(lx->OUT, "\r\n");
    }

    u_fprintf(lx->OUT, "<BR>\r\n");

    /* minimal days in week ================= */
    u_fprintf(lx->OUT, "%S", FSWF("DateTimeElements1", "Minimal Days in First Week: "));
  
    if(U_SUCCESS(status)) {
      u_fprintf(lx->OUT, " %d \r\n", elements[1]); 
    } else {
        explainStatus(lx, status, key);
        u_fprintf(lx->OUT, "\r\n");
    }

    u_fprintf(lx->OUT, "</TD>");

    showKeyAndEndItem(lx, key, locale);
    ures_close(array);
    ures_close(item);
}

void showShortLongCal( LXContext *lx, UResourceBundle *rb, const char *locale, const char *keyStem)
{
  char aKeyStem[400];
  char *q;
  strcpy(aKeyStem, keyStem);
  aKeyStem[0]=toupper(aKeyStem[0]);
  if((q = strstr(aKeyStem, "Names"))) {
    *q = 0;
  }
  /* dayNames -> Day,  monthNames -> Month 
     for legacy translations */
  showKeyAndStartItem(lx, aKeyStem, NULL, locale, FALSE, U_ZERO_ERROR); /* No status possible  because we have two items */

   u_fprintf(lx->OUT, "<h4>%S</h4>\n", FSWF("Calendar_type_format", "Formatting"));
   showShortLongCalType(lx, rb, locale, keyStem, "format");
   u_fprintf(lx->OUT, "<h4>%S</h4>\n", FSWF("Calendar_type_stand-alone", "Stand-alone"));
   showShortLongCalType(lx, rb, locale, keyStem, "stand-alone");

  u_fprintf(lx->OUT, "</td>");
  
  showKeyAndEndItem(lx, keyStem, locale);
}

/* Show a resource that has a short (*Abbreviations) and long (*Names) version ---------------- */
/* modified showArray */
void showShortLongCalType( LXContext *lx, UResourceBundle *rb, const char *locale, const char *keyStem, const char *type )
{
    UErrorCode status = U_ZERO_ERROR;
    /*UErrorCode shortStatus = U_ZERO_ERROR, longStatus = U_ZERO_ERROR;*/
    /*char       shortKey[100], longKey[100];*/
    /*UResourceBundle *item = NULL;*/
    /*int32_t len;*/
    /*const UChar *s  = 0;*/
    int i,j;
    int stuffCount;
    int maxCount = 0;
    struct {
      const char *style;
      const UChar *title;
      int32_t count;
      UResourceBundle *bund;
      UBool isDefault;
    } stuff[] = { {"narrow", NULL, -1, NULL, FALSE},
                  {"abbreviated", NULL, -1, NULL, FALSE},
                  {"wide", NULL, -1, NULL, FALSE} };
 
    stuffCount = sizeof(stuff)/sizeof(stuff[0]);
    stuff[0].title = FSWF("DayNarrow", "Narrow Names");
    stuff[1].title = FSWF("DayAbbreviations", "Short Names");
    stuff[2].title = FSWF("DayNames", "Long Names");

/* FSWF("MonthAbbreviations", " - NOT USED - see DayAbbreviations ") */
/* FSWF("MonthNames", "  - NOT USED - see DayNames ") */

    for(i=0;i<stuffCount;i++) {
      stuff[i].bund = loadCalRes3(lx, keyStem, type, stuff[i].style, &stuff[i].isDefault, &status);
      stuff[i].count = ures_getSize(stuff[i].bund);
      if(stuff[i].count > maxCount) {
        maxCount = stuff[i].count;
      }
    }

    if(U_FAILURE(status)) {
      explainStatus(lx, status, keyStem);
    } else { 
      u_fprintf(lx->OUT, "<table border=1 w_idth=\"100%%\"><tr><th>#</th>");
      maxCount =0; /* recount max */
      for(i=0;i<stuffCount;i++) {
        u_fprintf(lx->OUT, "<th>%S", stuff[i].title);
        if((strcmp(type,"format")||(i==0)) &&
           !strcmp(ures_getLocaleByType(stuff[i].bund,ULOC_ACTUAL_LOCALE,&status),"root") &&
           (!lx->curLocaleName[0]||strcmp(lx->curLocaleName,"root"))) {
          UChar tmp[2048]; /* FSWF is not threadsafe. Use a buffer */
          u_fprintf(lx->OUT, "<br>");
          if(strcmp(type,"format")) {
            u_sprintf(tmp, "%S type",  FSWF("Calendar_type_format", "Formatting"));
          } else if(i==0) {
            /* narrow (0) inherits from abbreviated (1) */
            u_strcpy(tmp, stuff[1].title);
          }
          u_fprintf_u(lx->OUT, FSWF(/**/"inherited_from", "from: %S"), tmp);
          stuff[i].count=0;
        } if(stuff[i].isDefault) {
          u_fprintf(lx->OUT, "<br>");
          calPrintDefaultWarning(lx);
        }
        if(stuff[i].count > maxCount) {
          maxCount = stuff[i].count;
        }
        u_fprintf(lx->OUT, "</th>");
      }
      u_fprintf(lx->OUT, "</tr>\n");

      for(j=0;j<maxCount;j++) {
        u_fprintf(lx->OUT, " <tr><td>%d</td>", j);
        for(i=0;i<stuffCount;i++) {
          if(i>=stuff[i].count) {
            u_fprintf(lx->OUT, "<td></td>");
          } else {
            const UChar *s;
            int32_t len;
            UErrorCode subStatus = U_ZERO_ERROR;
            s = ures_getStringByIndex(stuff[i].bund, j, &len, &subStatus);
            if(U_SUCCESS(subStatus) && len) {
              u_fprintf(lx->OUT, "<td>%S</td>", s);
            } else {
              u_fprintf(lx->OUT, "<td>");
              explainStatus(lx, subStatus, NULL);
              u_fprintf(lx->OUT, "</td>");
            }
          }
        }
        u_fprintf(lx->OUT, "</tr>\n");
      }

      u_fprintf(lx->OUT, "</table>\n");
    }
#if 0
    u_fprintf(lx->OUT, "<table border=1 w_idth=\"100%%\"><tr><td><b>#</b></td><td><b>%S</b> ", shortName);
    explainStatus(lx, shortStatus, keyStem);
    u_fprintf(lx->OUT, "</td><td><b>%S</b> ", longName);
    explainStatus(lx, longStatus, keyStem);
    u_fprintf(lx->OUT, "</td></tr>\r\n");

 
    for(i=0;i<num;i++)
    {
        char *key;

        u_fprintf(lx->OUT, " <tr><td>%d</td><td>", i);

        /* get the normal name */
        status = U_ZERO_ERROR;
        key = longKey;
        item = ures_getByIndex(longArray, i, item, &status);
        s    = ures_getString(item, &len, &status);

        if(i==0)
            longStatus = status;
  
        if(U_SUCCESS(status))
            u_fprintf(lx->OUT, " %S ", s);
        else
            explainStatus(lx, status, keyStem); /* if there was an error */

        u_fprintf(lx->OUT, "</td><td>");

        /* get the short name */
        status = U_ZERO_ERROR;
        key = shortKey;
        item = ures_getByIndex(shortArray, i, item, &status);
        s    = ures_getString(item, &len, &status);

        if(i==0) {
          shortStatus = status;
        }
  
        if(U_SUCCESS(status)) {
          u_fprintf(lx->OUT, " %S ", s);
        } else {
          explainStatus(lx, status, keyStem); /* if there was an error */
        }

        u_fprintf(lx->OUT, "</td></tr>");
    }

    u_fprintf(lx->OUT, "</table>");

#endif

    if(U_SUCCESS(status)) {
      for(i=0;i<stuffCount;i++) {
        ures_close(stuff[i].bund);
      }
    }
}

/* Show a 2d array  -------------------------------------------------------------------*/

void show2dArrayWithDescription( LXContext *lx, UResourceBundle *rb, const char *locale, const UChar *desc[], const char *key )
{
    UErrorCode status = U_ZERO_ERROR;
    UErrorCode firstStatus;
    const UChar *s  = 0;
    int32_t h,v;
    int32_t rows,cols;
    UBool bigString = FALSE; /* is it big? */
    UBool userRequested = FALSE; /* Did the user request this string? */
    UBool isTZ = FALSE; /* do special TZ processing */
    int32_t len;

    UResourceBundle *array = ures_getByKey(rb, key, NULL, &status);
    UResourceBundle *row   = ures_getByIndex(array, 0, NULL, &status);
    UResourceBundle *item = NULL;

    rows = ures_getSize(array);
    cols = ures_getSize(row);

#ifndef LX_NO_USE_UTIMZONE
    isTZ = !strcmp(key, "zoneStrings");
    if(isTZ)
        cols = 7;
#endif

    if(U_SUCCESS(status) && ((rows > kShow2dArrayRowCutoff) || (cols > kShow2dArrayColCutoff)) )
    {
        bigString = TRUE;
        userRequested = didUserAskForKey(lx, key);
    }

    showKeyAndStartItem(lx, key, NULL, locale, TRUE, status);

    if(bigString && !userRequested) /* it's hidden. */
    {
        /* WIERD!! outputting '&#' through UTF8 seems to be -> '?' or something */
        u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
    }
    else
    {
        if(bigString)
        {
            u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                locale,
                key,
                FSWF("bigStringHide", "Hide"));
        }

        firstStatus = status;  /* save this for the next column.. */

        if(U_SUCCESS(status))
        {
            u_fprintf(lx->OUT,"<table border=1>\r\n");

            /* print the top row */
            u_fprintf(lx->OUT,"<tr><td></td>");
            for(h=0;h<cols;h++)
            {
                if(!desc[h])
                    break;

                u_fprintf(lx->OUT, "<td><b>");
                if(h == 0)
                {
                    u_fprintf(lx->OUT, "<a target=lx_tz href=\"http://oss.software.ibm.com/cvs/icu/~checkout~/icu/docs/tz.htm?content-type=text/html\">");
                }
                u_fprintf(lx->OUT,"%S", desc[h]);
                if(h == 0)
                {
                    u_fprintf(lx->OUT, "</a>");
                }
                u_fprintf(lx->OUT, "</b></td>\r\n");
            }
            u_fprintf(lx->OUT,"</tr>\r\n");

            for(v=0;v<rows;v++)
            {
                const UChar *zn = NULL;

                row   = ures_getByIndex(array, v, row, &status);

                if(U_FAILURE(status)) {
                    u_fprintf(lx->OUT, "<TR><TD><B>ERR: ");
                    explainStatus(lx, status, NULL);
                    status = U_ZERO_ERROR;
                    continue;
                }

                u_fprintf(lx->OUT,"<tr><td><b>%d</b></td>", v);
                for(h=0;h<cols;h++)
                {
                    status = U_ZERO_ERROR;


#if 0 /* def LX_NO_USE_UTIMZONE */
                    if(isTZ && (h == 6))
                    {
                        UTimeZone *zone = utz_open(zn);

                        s = NULL;

                        if(zone == NULL)
                            s = FSWF("zoneStrings_open_failed", "<I>Unknown</I>");
                        else
                        {
                            s = utz_hackyGetDisplayName(zone);
                            utz_close(zone); /* s will be NULL, so nothing will get printed below. */
                        }
                    }
                    else
#endif
                    {
                        item   = ures_getByIndex(row, h, item, &status);
                        s = ures_getString(item, &len, &status);
                    }

                    if(isTZ && (h == 0)) /* save off zone for later use */
                        zn = s;

                    /*      if((h == 0) && (v==0))
                            firstStatus = status; */ /* Don't need to track firstStatus, countArrayItems should do that for us. */

                    if(U_SUCCESS(status) && s)
                        u_fprintf(lx->OUT, "<td>%S</td>\r\n", s);
                    else
                    {
                        u_fprintf(lx->OUT, "<TD BGCOLOR=" kStatusBG " VALIGN=TOP>");
                        explainStatus(lx, status, key);
                        u_fprintf(lx->OUT, "</TD>\r\n");
                        break;
                    }
                }
                u_fprintf(lx->OUT, "</tr>\r\n");
            }
            u_fprintf(lx->OUT, "</table>\r\n<BR>");
        }
    }

    ures_close(item);
    ures_close(row);
    ures_close(array);
    showKeyAndEndItem(lx, key, locale);
}

/* Show a Tagged Array  -------------------------------------------------------------------*/

void showTaggedArray( LXContext *lx, UResourceBundle *rb, const char *locale, const char *key, UBool compareToDisplay )
{
  UErrorCode status = U_ZERO_ERROR;
  UErrorCode firstStatus;
  const UChar *s  = 0;
  int32_t v;
  int32_t rows;
  UBool bigString = FALSE; /* is it big? */
  UBool userRequested = FALSE; /* Did the user request this string? */
  int32_t len;
  UResourceBundle *item = NULL;

  rows = ures_countArrayItems(rb, key, &status);

  if(U_SUCCESS(status) && ((rows > kShow2dArrayRowCutoff))) {
    bigString = TRUE;
    userRequested = didUserAskForKey(lx, key);
  }

  showKeyAndStartItem(lx, key, NULL, locale, TRUE, status);

  if(bigString && !userRequested) /* it's hidden. */  {
    /* WIERD!! outputting '&#' through UTF8 seems to be -> '?' or something */
    u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
  } else {
    if(bigString) {
      u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                locale,
                key,
                FSWF("bigStringHide", "Hide"));
        }
        
        firstStatus = status;  /* save this for the next column.. */
        
        if(U_SUCCESS(status)) {	
            UResourceBundle *tagged =  ures_getByKey(rb, key, NULL, &status);
            UResourceBundle *defaultTagged = NULL;
            UResourceBundle *taggedItem = NULL;
            if(lx->dispRB) {
                defaultTagged =  ures_getByKey(lx->dispRB, key, NULL, &status);
            }
            u_fprintf(lx->OUT,"<table border=1>\r\n");
            
            /* print the top row */
            u_fprintf(lx->OUT,"<tr><td><b>%S</b></td>",
                FSWF("taggedarrayTag", "Tag"));
            
            if(compareToDisplay) {
                u_fprintf(lx->OUT, "<td><i>%S</i></td>",
                    defaultLanguageDisplayName(lx));
            }
            
            u_fprintf(lx->OUT, "<td><b>%S</b></td></tr>",
                lx->curLocale ? lx->curLocale->ustr : FSWF("none","None"));
            
            for(v=0;v<rows;v++) {
                const char *tag;
                
                status = U_ZERO_ERROR;
                taggedItem = ures_getByIndex(tagged, v, NULL, &status);
                tag = ures_getKey(taggedItem);
                
                /*tag = ures_getTaggedArrayTag(rb, key, v, &status);*/
                if(!tag)
                    break;
                
                u_fprintf(lx->OUT,"<tr> ");
                
                if(U_SUCCESS(status)) {
                    u_fprintf(lx->OUT, "<td><tt>%s</tt></td> ", tag);
                    
                    
                    if(compareToDisplay) {
                        if(lx->dispRB) {
                            item = ures_getByKey(defaultTagged, tag, item, &status);
                            s = ures_getString(item, &len, &status);
                            
                            if(s)
                                u_fprintf(lx->OUT, "<td><i>%S</i></td>", s);
                            else
                                u_fprintf(lx->OUT, "<td></td>");
                        } else {
                            u_fprintf(lx->OUT, "<td></td>");
                        }
                    }
                    
                    status = U_ZERO_ERROR;
                    switch(ures_getType(taggedItem)) {
                        
                    case URES_STRING:
                        s = ures_getString(taggedItem, &len, &status);
                        
                        if(s) {
#if 0
                          /* attempt to Bidi-shape the string if Arabic */
                            UChar junk[8192];
                            UErrorCode she=U_ZERO_ERROR;
                            int32_t dstl;
                            int32_t j;
                            UChar temp;
                            
                            dstl=u_shapeArabic(s, u_strlen(s), junk, 8192, U_SHAPE_LETTERS_SHAPE, &she);
                            junk[dstl]=0;
                            /* Reverses the string */
                            for (j = 0; j < (dstl / 2); j++){
                                temp = junk[(dstl-1) - j];
                                junk[(dstl-1) - j] = junk[j];
                                junk[j] = temp;
                            }
                            u_fprintf(lx->OUT, "<td>%S [a]</td>", junk);
#else
                            u_fprintf(lx->OUT, "<td>%S</td>", s);
#endif
                        } else {
                            u_fprintf(lx->OUT, "<TD BGCOLOR=" kStatusBG " VALIGN=TOP>");
                            explainStatus(lx, status, key);
                            u_fprintf(lx->OUT, "</TD>\r\n");
                        }
                        break;
                        
                    case URES_ARRAY:
                        {
                            UResourceBundle *subItem = NULL;
                            while((s = ures_getNextString(taggedItem, &len, NULL, &status)) && U_SUCCESS(status)) {
                                u_fprintf(lx->OUT, "<td>%S</td>", s);
                            }
                            
                            if(U_FAILURE(status) && (status != U_INDEX_OUTOFBOUNDS_ERROR)) {
                                u_fprintf(lx->OUT, "<TD BGCOLOR=" kStatusBG " VALIGN=TOP>");
                                explainStatus(lx, status, key);
                                u_fprintf(lx->OUT, "</TD>\r\n");
                            }
                            ures_close(subItem);
                        }
                        break;
                        
                    default:
                        u_fprintf(lx->OUT, "<TD><i>unknown resource type=%d</i></TD>", ures_getType(taggedItem));
                    } /* switch */
                }
                u_fprintf(lx->OUT, "</tr>\r\n");
            }
            u_fprintf(lx->OUT, "</table>\r\n<br>");
            ures_close(taggedItem); /* todo: mem. management? */
    }
  }
  
  u_fprintf(lx->OUT, "</td>");
  showKeyAndEndItem(lx, key, locale);
}


void showCurrencies( LXContext *lx, UResourceBundle *rb, const char *locale )
{
  UErrorCode status = U_ZERO_ERROR;
  UErrorCode firstStatus;
  const UChar *s  = 0;
  int32_t v;
  int32_t rows;
  UBool bigString = FALSE; /* is it big? */
  UBool userRequested = FALSE; /* Did the user request this string? */
  int32_t len;
  const char *key = "Currencies";
  UChar  cflu[9] = { 0, 0, };
  char cfl[4] = {0};
  UBool sawDefault = FALSE;

  rows = ures_countArrayItems(rb, key, &status);

  if(U_SUCCESS(status) && ((rows > kShow2dArrayRowCutoff))) {
    bigString = TRUE;
    userRequested = didUserAskForKey(lx, key);
  }

  showKeyAndStartItem(lx, key, NULL, locale, TRUE, status);

  if(U_SUCCESS(status)) {
    UErrorCode defCurSt = U_ZERO_ERROR;
    ucurr_forLocale(locale, cflu, sizeof(cflu)-1, &defCurSt);
    if(U_FAILURE(defCurSt) || !cflu[0]) {
      u_fprintf(lx->OUT, "%S: <!-- for %s -->", FSWF("currNoDefault", "No Default Currency"), locale);
      explainStatus(lx,defCurSt,key);
      u_fprintf(lx->OUT, "<BR>\r\n");
    } else {
      u_UCharsToChars(cflu, cfl, 4);
      cfl[3]=0;
    }
  }

  if(bigString && !userRequested) /* it's hidden. */  {
    u_fprintf(lx->OUT, "<A HREF=\"?_=%s&SHOW%s=1#%s\"><IMG BORDER=0 WIDTH=16 HEIGHT=16 SRC=\"" LDATA_PATH "closed.gif\" ALT=\"\">%S</A>\r\n<P>", locale, key,key, FSWF("bigStringClickToShow","(Omitted due to size. Click here to show.)"));
  } else {
    if(bigString) {
      u_fprintf(lx->OUT, "<A HREF=\"?_=%s#%s\"><IMG border=0 width=16 height=16 SRC=\"" LDATA_PATH "opened.gif\" ALT=\"\"> %S</A><P>\r\n",
                locale,
                key,
                FSWF("bigStringHide", "Hide"));
    }
      
    firstStatus = status;  /* save this for the next column.. */
      
    if(U_SUCCESS(status)) {	
      UResourceBundle *tagged =  ures_getByKey(rb, key, NULL, &status);
      UResourceBundle *defaultTagged = NULL;
      UResourceBundle *taggedItem = NULL;
      if(lx->dispRB) {
        defaultTagged =  ures_getByKey(lx->dispRB, key, NULL, &status);
      }
      u_fprintf(lx->OUT,"<table border=1>\r\n");
        
      /* print the top row */
      u_fprintf(lx->OUT,"<tr><th>%S</th><th>%S</th><th>%S</th><th>%S</th>",
                FSWF("currCode", "Code"),
                FSWF("currSymbol", "Symbol"),
                FSWF("currName", "Name"),
                FSWF("currDigits", "Decimal Digits"));
        
      for(v=0;v<rows;v++) {
        const char *tag;
	UBool isDefault = FALSE;
          
        status = U_ZERO_ERROR;
        taggedItem = ures_getByIndex(tagged, v, NULL, &status);
        tag = ures_getKey(taggedItem);
          
        if(!tag)
          break;

	if(!strcmp(tag,cfl)) {
		isDefault = TRUE;
	}
          
        u_fprintf(lx->OUT,"<tr> ");
          
        if(U_SUCCESS(status)) {
          u_fprintf(lx->OUT, "<TD><TT>%s%s%s</TT></TD> ", 
                    isDefault?"<b>":"",
                    tag,
                    isDefault?"</b>":"");
                    
          if(isDefault) {
            sawDefault = TRUE;
          }
            
          status = U_ZERO_ERROR;
          switch(ures_getType(taggedItem)) {
              
          case URES_STRING:  /* old format ICU data */
            s = ures_getString(taggedItem, &len, &status);
              
            if(s) {
              u_fprintf(lx->OUT, "<td>%S</td>", s);
            } else {
              u_fprintf(lx->OUT, "<TD BGCOLOR=" kStatusBG " VALIGN=TOP>");
              explainStatus(lx, status, key);
              u_fprintf(lx->OUT, "</TD>\r\n");
            }
            break;
              
          case URES_ARRAY:
            {
              UResourceBundle *subItem = NULL;
              while((s = ures_getNextString(taggedItem, &len, NULL, &status)) && U_SUCCESS(status)) {
                u_fprintf(lx->OUT, "<td>%S</td>", s);
              }
                
              if(U_FAILURE(status) && (status != U_INDEX_OUTOFBOUNDS_ERROR)) {
                u_fprintf(lx->OUT, "<TD BGCOLOR=" kStatusBG " VALIGN=TOP>");
                explainStatus(lx, status, key);
                u_fprintf(lx->OUT, "</TD>\r\n");
              }
              ures_close(subItem);
            }
            break;
              
          default:
            u_fprintf(lx->OUT, "<TD><i>unknown resource type=%d</i></TD>", ures_getType(taggedItem));
          } /* switch */
        }
          
        /* Currency additions */
        {
          UChar ucn[8];
          u_charsToUChars(tag, ucn,4);
          u_fprintf(lx->OUT, "<td>%d</td>", ucurr_getDefaultFractionDigits(ucn, &status));
        }

        if(isDefault) {
            u_fprintf(lx->OUT, "<td><B>%S</B></td>",
              FSWF("currDefault","Default Currency for this locale"));
        }

        u_fprintf(lx->OUT, "</TR>\r\n");
      }
      if(!sawDefault && cflu[0]) {
        UBool isChoiceFormat = FALSE;
        int32_t len = 0;
        UErrorCode subSta = U_ZERO_ERROR;
        u_fprintf(lx->OUT, "<tr><td><b>%S</b></td><td><b>%S</b></td><td><b>%S</b></td><td>%d</td>\r\n", 
                  cflu,
                  ucurr_getName(cflu,locale,UCURR_SYMBOL_NAME,&isChoiceFormat,&len,&subSta),
                  ucurr_getName(cflu,locale,UCURR_LONG_NAME,&isChoiceFormat,&len,&subSta),
                  ucurr_getDefaultFractionDigits(cflu, &status)
                  );
        if(U_FAILURE(subSta)) {
          u_fprintf(lx->OUT, "<td>");
          explainStatus(lx, subSta, key);
          u_fprintf(lx->OUT, "</td>");
        }
        u_fprintf(lx->OUT, "<td><B>%S</B><br>",
              FSWF("currDefault","Default Currency for this locale"));
        u_fprintf(lx->OUT, "<i>%S</i></td>\r\n", FSWF("currNotInLoc", "Note: localization for this currency was not found in this locale"));
        u_fprintf(lx->OUT, "</tr>\r\n");
      }
      u_fprintf(lx->OUT, "</table>\r\n<br>");
      ures_close(taggedItem); /* todo: mem. management? */
    }
  }
  u_fprintf(lx->OUT, "</td>");
  showKeyAndEndItem(lx, key, locale);
}

UResourceBundle *loadCalRes(LXContext *lx, const char *keyStem, UBool *isDefault, UErrorCode *status) {
  /* Yes, this is a near-reimplementation of icu::CalendarData.  */
  UResourceBundle *item1 = NULL;
  *isDefault = FALSE;
  if(U_FAILURE(*status)) { return NULL; }
  if(!lx->calMyBundle) {
#if defined(LX_DEBUG)
    fprintf(stderr, "loadCalRes - no calMyBundle ! \n");
#endif
    *status = U_INTERNAL_PROGRAM_ERROR;
    return NULL;
  } else {
    item1 = ures_getByKeyWithFallback(lx->calMyBundle, keyStem, item1, status);
  }
  
  if((*status == U_MISSING_RESOURCE_ERROR) && (lx->calFbBundle)) {
    *status = U_ZERO_ERROR;
    *isDefault = TRUE;
    item1 = ures_getByKeyWithFallback(lx->calFbBundle, keyStem, item1, status);
  }

  if(U_FAILURE(*status)) {
    ures_close(item1);
    return NULL;
  } else {
    return item1;
  }
}


UResourceBundle *loadCalRes3(LXContext *lx, const char *keyStem, const char *type, const char *style, UBool *isDefault, UErrorCode *status) {
  /* Yes, this is a near-reimplementation of icu::CalendarData.  */
  UResourceBundle *item1 = NULL;
  UResourceBundle *item2 = NULL;
  UResourceBundle *item3 = NULL;
  *isDefault = FALSE;
  if(U_FAILURE(*status)) { return NULL; }
  if(!lx->calMyBundle) {
#if defined(LX_DEBUG)
    fprintf(stderr, "loadCalRes3 - no calMyBundle ! \n");
#endif
    *status = U_INTERNAL_PROGRAM_ERROR;
    return NULL;
  } else {
    item1 = ures_getByKeyWithFallback(lx->calMyBundle, keyStem, item1, status);
    item2 = ures_getByKeyWithFallback(item1, type, item2, status);
    item3 = ures_getByKeyWithFallback(item2, style, item3, status);
  }
  
  if((*status == U_MISSING_RESOURCE_ERROR) && (lx->calFbBundle)) {
    *status = U_ZERO_ERROR;
    *isDefault = TRUE;
    item1 = ures_getByKeyWithFallback(lx->calFbBundle, keyStem, item1, status);
    item2 = ures_getByKeyWithFallback(item1, type, item2, status);
    item3 = ures_getByKeyWithFallback(item2, style, item3, status);
  }

  ures_close(item1);
  ures_close(item2);
  if(U_FAILURE(*status)) {
    ures_close(item3);
    return NULL;
  } else {
    return item3;
  }
}

void calPrintDefaultWarning(LXContext *lx) {
  UErrorCode status = U_ZERO_ERROR;

  /*UChar keyBuf[1024];*/
    UChar valBuf[1024];
    char loc[1024];
/*     keyBuf[0]=0; */
/*     uloc_getDisplayKeyword("calendar", */
/*                            lx->dispLocale, */
/*                            keyBuf, */
/*                            1024, */
/*                            &status); */
    sprintf(loc, "@%s=%s", "calendar", "gregorian");
    uloc_getDisplayKeywordValue(loc,
                                "calendar",
                                lx->dispLocale,
                                valBuf,
                                1024,
                                &status);
    u_fprintf(lx->OUT, "<font size=-2>");
    u_fprintf_u(lx->OUT, FSWF(/**/"inherited_from", "from: %S"), valBuf);
    u_fprintf(lx->OUT, "</font>\n");
}

void showDefaultCalendar(LXContext *lx, UResourceBundle *myRB, const char *locale) {
  /*const char *urlCal = lx->curLocaleBlob.calendar;*/
  UErrorCode status = U_ZERO_ERROR;
  UResourceBundle *fillin1 = NULL;
  UResourceBundle *fillin2 = NULL;
  const char *key = "DefaultCalendar";
  const UChar *s;
  int32_t len;

  strcpy(lx->defaultCalendar, "gregorian");

  fillin1 = ures_getByKey(myRB, "calendar", fillin1, &status);
  fillin2 = ures_getByKeyWithFallback(fillin1, "default", fillin2, &status);

  showKeyAndStartItem(lx, key, NULL, locale, FALSE, status);

  if(U_SUCCESS(status)) {
    s  = ures_getString(fillin2, &len, &status);
    if (s) {
      {
        char defCalStr[200];
        if(len > 199) {
          len = 199;
        }
        u_UCharsToChars(s, defCalStr, len);
        defCalStr[len]=0;
        if(defCalStr[0]) {
          strcpy(lx->defaultCalendar,defCalStr);
        }
      }
    }
  }

  if(U_SUCCESS(status)) {
    UChar keyBuf[1024];
    UChar valBuf[1024];
    char loc[1024];
    keyBuf[0]=0;
    uloc_getDisplayKeyword("calendar",
                           lx->dispLocale,
                           keyBuf,
                           1024,
                           &status);
    sprintf(loc, "@%s=%s", "calendar", lx->defaultCalendar);
    uloc_getDisplayKeywordValue(loc,
                                "calendar",
                                lx->dispLocale,
                                valBuf,
                                1024,
                                &status);
    u_fprintf(lx->OUT, "%S %S: %S<br>", FSWF("keyword_Default", "Default"), keyBuf, valBuf);
    if(lx->curLocaleBlob.calendar[0]) {
      sprintf(loc, "@%s=%s", "calendar", lx->curLocaleBlob.calendar);
      uloc_getDisplayKeywordValue(loc,
                                  "calendar",
                                  lx->dispLocale,
                                  valBuf,
                                  1024,
                                  &status);
      u_fprintf(lx->OUT, "%S %S: %S<br>", FSWF("keyword_Current", "Current"), keyBuf, valBuf);
    }
  }
  /* user selection overrides default */
  if(lx->curLocaleBlob.calendar[0]) {
    strcpy(lx->defaultCalendar, lx->curLocaleBlob.calendar);
  }

  u_fprintf(lx->OUT, "</TD>");
  showKeyAndEndItem(lx, key, locale);
}


void showDateTime(LXContext *lx, UResourceBundle *myRB, const char *locale)
{
  UErrorCode status = U_ZERO_ERROR;
  /*UBool typeFallback = FALSE;*/
  UResourceBundle *calBundle = NULL; /* "calendar" */
  /*UResourceBundle *myBundle = NULL;*/ /* 'type' */
  /*UResourceBundle *fbBundle = NULL;*/ /* gregorian */
    
  /* %%%%%%%%%%%%%%%%%%%%%%%*/
  /*   Date/Time section %%%*/
  showDefaultCalendar(lx, myRB, locale); /* and setup lx->defaultCalendar */

  calBundle = ures_getByKey(myRB, "calendar", NULL, &status);

  if(U_FAILURE(status)) {
    u_fprintf(lx->OUT, "Can't load 'calendar': ");
    explainStatus(lx, status, "calendar");
    return;
  }

  if(!strcmp(lx->defaultCalendar,"gregorian")) {
    lx->calMyBundle = ures_getByKeyWithFallback(calBundle, "gregorian", NULL, &status);
    if(U_FAILURE(status)) {
      u_fprintf(lx->OUT, "Can't load 'calendar/%s': ", lx->defaultCalendar);
      explainStatus(lx, status, "calendar");
      return;
    }
  } else {
    lx->calMyBundle = ures_getByKeyWithFallback(calBundle, lx->defaultCalendar, NULL, &status);
    if(U_FAILURE(status)) {
      u_fprintf(lx->OUT, "Can't load 'calendar/%s': ", lx->defaultCalendar);
      explainStatus(lx, status, "calendar");
      return;
    }
    lx->calFbBundle = ures_getByKeyWithFallback(calBundle, "gregorian", NULL, &status);
    if(U_FAILURE(status)) {
      u_fprintf(lx->OUT, "Can't load 'calendar/%s': ", "gregorian");
      explainStatus(lx, status, "calendar");
      return;
    }
  }

  if(U_FAILURE(status)) {
    explainStatus(lx, status, "calendar");
    return;
  }

  showShortLongCal(lx, myRB, locale, "dayNames"); 
  showShortLongCal(lx, myRB, locale, "monthNames");
  
  u_fprintf(lx->OUT, "&nbsp;<table cellpadding=0 cellspacing=0 width=\"100%%\"><tr><td VALIGN=\"TOP\">");
  
  {
    const UChar *ampmDesc[3];
    ampmDesc[0] = FSWF("AmPmMarkers0", "am");
    ampmDesc[1] = FSWF("AmPmMarkers1", "pm");
    ampmDesc[2] = 0;
    
    showArrayWithDescription(lx, myRB, locale, ampmDesc, "AmPmMarkers", kCal);
  }
  u_fprintf(lx->OUT, "</td><td>&nbsp;</td><td VALIGN=\"TOP\">");
  showArray(lx, myRB, locale, "eras", kCal);
  u_fprintf(lx->OUT, "</td></tr></table>");
  
  
  {
    const UChar *dtpDesc[10]; /* flms */
    dtpDesc[0] = FSWF("DateTimePatterns0", "Full Time");
    dtpDesc[1] = FSWF("DateTimePatterns1", "Long Time");
    dtpDesc[2] = FSWF("DateTimePatterns2", "Medium Time");
    dtpDesc[3] = FSWF("DateTimePatterns3", "Short Time");
    dtpDesc[4] = FSWF("DateTimePatterns4", "Full Date");
    dtpDesc[5] = FSWF("DateTimePatterns5", "Long Date");
    dtpDesc[6] = FSWF("DateTimePatterns6", "Medium Date");
    dtpDesc[7] = FSWF("DateTimePatterns7", "Short Date");
    dtpDesc[8] = FSWF("DateTimePatterns8", "Date-Time pattern.<BR>{0} = time, {1} = date");
    dtpDesc[9] = 0;
    
    showArrayWithDescription(lx, myRB, locale, dtpDesc, "DateTimePatterns", kCal);
  }
  
  
  { 
    const UChar *zsDesc[8];
    zsDesc[0] = FSWF("zoneStrings0", "Canonical");
    zsDesc[1] = FSWF("zoneStrings1", "Normal Name");
    zsDesc[2] = FSWF("zoneStrings2", "Normal Abbrev");
    zsDesc[3] = FSWF("zoneStrings3", "Summer/DST Name");
    zsDesc[4] = FSWF("zoneStrings4", "Summer/DST Abbrev");
    zsDesc[5] = FSWF("zoneStrings5", "Example City");
#ifndef LX_NO_USE_UTIMZONE
    zsDesc[6] = FSWF("zoneStrings6", "Raw Offset");
#else
    zsDesc[6] = 0;
#endif
    zsDesc[7] = 0;
    
    show2dArrayWithDescription(lx, myRB, locale, zsDesc, "zoneStrings");  /* not calendrical */
  }
  showLPC(lx, myRB, locale);
  showDateTimeElements(lx, myRB, locale); /* not calendrical? */
}

/* locale pattern chars */
void showLPC(LXContext *lx, UResourceBundle *myRB, const char *locale)
{
    const UChar *charDescs[24];
    
    charDescs[0] = FSWF("localPatternChars0", "Era");
    charDescs[1] = FSWF("localPatternChars1", "Year");
    charDescs[2] = FSWF("localPatternChars2", "Month");
    charDescs[3] = FSWF("localPatternChars3", "Day of Month");
    charDescs[4] = FSWF("localPatternChars4", "Hour Of Day 1");
    charDescs[5] = FSWF("localPatternChars5", "Hour Of Day 0"); 
    charDescs[6] = FSWF("localPatternChars6", "Minute");
    charDescs[7] = FSWF("localPatternChars7", "Second");
    charDescs[8] = FSWF("localPatternChars8", "Millisecond");
    charDescs[9] = FSWF("localPatternChars9", "Day Of Week");
    charDescs[10] = FSWF("localPatternChars10", "Day Of Year");
    charDescs[11] = FSWF("localPatternChars11", "Day Of Week In Month");
    charDescs[12] = FSWF("localPatternChars12", "Week Of Year");
    charDescs[13] = FSWF("localPatternChars13", "Week Of Month");
    charDescs[14] = FSWF("localPatternChars14", "Am/Pm");
    charDescs[15] = FSWF("localPatternChars15", "Hour 1");
    charDescs[16] = FSWF("localPatternChars16", "Hour 0"); 
    charDescs[17] = FSWF("localPatternChars17", "Timezone");
    charDescs[18] = FSWF("localPatternChars18", "Year (of 'Week of Year')");
    charDescs[19] = FSWF("localPatternChars19", "Day of Week (1=first day according to locale)"); 
    charDescs[20] = FSWF(/**/"localPatternChars20", "extended year");
    charDescs[21] = FSWF(/**/"localPatternChars21", "julian day");
    charDescs[22] = FSWF(/**/"localPatternChars22", "millis in day");
    charDescs[23] = FSWF(/**/"localPatternChars23", "timezone rfc");
    charDescs[24] = 0;
    
    showStringWithDescription(lx, myRB, locale, charDescs, "localPatternChars", TRUE); /* calendrical? */
}
