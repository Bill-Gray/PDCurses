/*
 * Copyright (C) 1998-2001  Mark Hessling <M.Hessling@qut.edu.au>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

static char RCSid[] = "$Id: rxpack.c,v 1.8 2002/07/29 06:19:40 mark Exp $";

#include "rxpack.h"


#if !defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
static RexxExitHandler RxExitHandlerForSayTraceRedirection;
#endif

#if defined(USE_REXX6000)
static LONG RxSubcomHandler( RSH_ARG0_TYPE, RSH_ARG1_TYPE, RSH_ARG2_TYPE );
#else
static RexxSubcomHandler RxSubcomHandler;
#endif

#if !defined(HAVE_STRERROR)
/*
 * This function and the following description borrowed from Regina 0.08a
 *
 * Sigh! This must probably be done this way, although it's incredibly 
 * backwards. Some versions of gcc comes with a complete set of ANSI C
 * include files, which contains the definition of strerror(). However,
 * that function does not exist in the default libraries of SunOS. 
 * To circumvent that problem, strerror() is #define'd to get_sys_errlist() 
 * in config.h, and here follows the definition of that function. 
 * Originally, strerror() was #defined to sys_errlist[x], but that does
 * not work if string.h contains a declaration of the (non-existing) 
 * function strerror(). 
 *
 * So, this is a mismatch between the include files and the library, and
 * it should not create problems for Regina. However, the _user_ will not
 * encounter any problems until he compiles Regina, so we'll have to 
 * clean up after a buggy installation of the C compiler!
 */
char *rxpackage_get_sys_errlist( int num )
{
   extern char *sys_errlist[] ;
   return sys_errlist[num] ;
}
#endif


/*-----------------------------------------------------------------------------
 * Compares buffers for equality ignoring case
 *----------------------------------------------------------------------------*/
int memcmpi

#ifdef HAVE_PROTO
   (char *buf1, char *buf2, int len)
#else
   (buf1,buf2,len)
   char    *buf1;
   char    *buf2;
   int     len;
#endif
{
   register short i=0;
   char c1=0,c2=0;
   for(i=0;i<len;i++)
   {
      if (isupper(*buf1))
         c1 = tolower(*buf1);
      else
         c1 = *buf1;
      if (isupper(*buf2))
         c2 = tolower(*buf2);
      else
         c2 = *buf2;
      if (c1 != c2)
         return(c1-c2);
      ++buf1;
      ++buf2;
   }
 return(0);
}
/*-----------------------------------------------------------------------------
 * Uppercases the supplied string.
 *----------------------------------------------------------------------------*/
char *make_upper

#ifdef HAVE_PROTO
   (char *str)
#else
   (str)
   char    *str;
#endif

{
   char *save_str=str;
   while(*str)
   {
      if (islower(*str))
         *str = toupper(*str);
      ++str;
   }
   return(save_str);
}

/*-----------------------------------------------------------------------------
 * Allocate memory for a char * based on an RXSTRING
 *----------------------------------------------------------------------------*/
char *AllocString

#ifdef HAVE_PROTO
   (char *buf, int bufsize)
#else
   (buf, bufsize)
   char    *buf;
   int   bufsize;
#endif
{
   char *tempstr=NULL;

   tempstr = (char *)malloc(sizeof(char)*(bufsize+1));
   return tempstr;
}


/*-----------------------------------------------------------------------------
 * Copy a non terminated character array to the nominated buffer (truncate
 * if necessary) and null terminate.
 *----------------------------------------------------------------------------*/
char *MkAsciz

#ifdef HAVE_PROTO
   (char *buf, int bufsize, char *str, int len)
#else
   (buf, bufsize, str, len)
   char    *buf;
   int  bufsize;
   char    *str;
   int  len;
#endif

{
   bufsize--;     /* Make room for terminating byte */
   if (len > bufsize)
      len = bufsize;
   (void)memcpy(buf, str, len);
   buf[len] = '\0';
   return buf;
}

/*-----------------------------------------------------------------------------
 * Check number of parameters
 *----------------------------------------------------------------------------*/
int my_checkparam(RxPackageGlobalDataDef *RxPackageGlobalData, const char *name, int argc, int mini, int maxi)
{
   if ( argc < mini )
   {
      RxDisplayError( RxPackageGlobalData,
                      name,
                      "*ERROR* Not enough parameters in call to \"%s\". Minimum %d\n",
                      name, mini);
      return 1;
   }
   if ( argc > maxi
   &&   maxi != 0 )
   {
      RxDisplayError( RxPackageGlobalData,
                      name, 
                      "*ERROR* Too many parameters in call to \"%s\". Maximum %d\n",
                      name, maxi);
      return 1;
   }
   return 0;
}

/*-----------------------------------------------------------------------------
 * Create a REXX variable of the specified name and bind the value to it.
 *----------------------------------------------------------------------------*/
int SetRexxVariable

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, int namelen, char *value, int valuelen )
#else
   ( RxPackageGlobalData, name, namelen, value, valuelen )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char    *name;
   int     namelen;
   char    *value;
   int     valuelen;
#endif

{
   ULONG  rc=0L;
   SHVBLOCK       shv;

   InternalTrace( RxPackageGlobalData, "SetRexxVariable", "\"%s\",%d,\"%s\",%d", name, namelen, value, valuelen );

   if ( RxPackageGlobalData->RxRunFlags & MODE_DEBUG)
   {
      char buf1[50], buf2[50];

      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
         "*DEBUG* Setting variable \"%s\" to \"%s\".\n",
         MkAsciz( buf1, sizeof( buf1 ), name, namelen ),
         MkAsciz( buf2, sizeof( buf2 ), value, valuelen ) );
   }
   ( void )make_upper( name );
   shv.shvnext = ( SHVBLOCK* )NULL;
   shv.shvcode = RXSHV_SET;
   MAKERXSTRING( shv.shvname, name, ( ULONG )namelen );
   MAKERXSTRING( shv.shvvalue, value, ( ULONG )valuelen );
   shv.shvnamelen = shv.shvname.strlength;
   shv.shvvaluelen = shv.shvvalue.strlength;
   assert( shv.shvname.strptr );
   assert( shv.shvvalue.strptr );
   rc = RexxVariablePool( &shv );
   if ( rc == RXSHV_OK
   ||   rc == RXSHV_NEWV )
      return( 0 );
   else
      return( 1 );
}

/*-----------------------------------------------------------------------------
 * This function gets a REXX variable and returns it in an RXSTRING
 *----------------------------------------------------------------------------*/
RXSTRING *GetRexxVariable

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, RXSTRING *value, int suffix )
#else
   ( RxPackageGlobalData, name, value, suffix )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
   RXSTRING *value;
   int suffix;
#endif
{
   static SHVBLOCK shv;
   char variable_name[350];
   ULONG rc = 0L;

   InternalTrace( RxPackageGlobalData, "GetRexxVariable", "%s,%x,%d", name, value, suffix );

   shv.shvnext=NULL;                                /* only one block */
   shv.shvcode = RXSHV_FETCH;
/*
 * This calls the RexxVariablePool() function for each value. This is
 * not the most efficient way of doing this.
 */
   if ( suffix == -1)
      strcpy( variable_name, name );
   else
      sprintf( variable_name, "%s%-d", name, suffix );
   ( void )make_upper( variable_name );
/*
 * Now (attempt to) get the REXX variable
 * Set shv.shvvalue to NULL to force interpreter to allocate memory.
 */
   assert( variable_name );
   MAKERXSTRING( shv.shvname, variable_name, strlen( variable_name ) );
   assert( shv.shvname.strptr );
   shv.shvvalue.strptr = NULL;
   shv.shvvalue.strlength = 0;
/*
 * One or both of these is needed, too <sigh>
 */
   shv.shvnamelen = strlen( variable_name );
   shv.shvvaluelen = 0;
   rc = RexxVariablePool( &shv );              /* Set the REXX variable */
   if ( rc == RXSHV_OK )
   {
      assert( value );
      value->strptr = ( RXSTRING_STRPTR_TYPE )malloc( ( sizeof( char )*shv.shvvalue.strlength ) + 1 );
      if ( value->strptr != NULL )
      {
         value->strlength = shv.shvvalue.strlength;
         memcpy( value->strptr, shv.shvvalue.strptr, value->strlength );
         *( value->strptr + value->strlength ) = '\0';
      }
#if defined(REXXFREEMEMORY)
      RexxFreeMemory( shv.shvvalue.strptr );
#elif defined(WIN32) && (defined(USE_OREXX) || defined(USE_WINREXX) || defined(USE_QUERCUS))
      GlobalFree( shv.shvvalue.strptr );
#elif defined(USE_OS2REXX)
      DosFreeMem( shv.shvvalue.strptr );
#else
      free( shv.shvvalue.strptr );
#endif
   }
   else
      value = NULL;
   return( value );
}

/*-----------------------------------------------------------------------------
 * This function gets a REXX variable as a number and returns it
 * Returns NULL if the variable is not set; an invalid number returns 0
 *----------------------------------------------------------------------------*/
int *GetRexxVariableInteger

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, int *value, int suffix )
#else
   ( RxPackageGlobalData, name, value, suffix )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
   int *value;
   int suffix;
#endif
{
   static SHVBLOCK shv;
   char variable_name[350];
   ULONG rc = 0L;

   InternalTrace( RxPackageGlobalData, "GetRexxVariableNumber", "%s,%x,%d", name, value, suffix );

   shv.shvnext=NULL;                                /* only one block */
   shv.shvcode = RXSHV_FETCH;
/*
 * This calls the RexxVariablePool() function for each value. This is
 * not the most efficient way of doing this.
 */
   if ( suffix == -1)
      strcpy( variable_name, name );
   else
      sprintf( variable_name, "%s%-d", name, suffix );
   ( void )make_upper( variable_name );
/*
 * Now (attempt to) get the REXX variable
 * Set shv.shvvalue to NULL to force interpreter to allocate memory.
 */
   assert( variable_name );
   MAKERXSTRING( shv.shvname, variable_name, strlen( variable_name ) );
   assert( shv.shvname.strptr );
   shv.shvvalue.strptr = NULL;
   shv.shvvalue.strlength = 0;
/*
 * One or both of these is needed, too <sigh>
 */
   shv.shvnamelen = strlen( variable_name );
   shv.shvvaluelen = 0;
   rc = RexxVariablePool( &shv );              /* Get the REXX variable */
   if ( rc == RXSHV_OK )
   {
      assert( value );
      if ( StrToInt( &shv.shvvalue, (ULONG *)value ) == -1 )
         value = NULL;
#if defined(REXXFREEMEMORY)
      RexxFreeMemory( shv.shvvalue.strptr );
#elif defined(WIN32) && (defined(USE_OREXX) || defined(USE_WINREXX) || defined(USE_QUERCUS))
      GlobalFree( shv.shvvalue.strptr );
#elif defined(USE_OS2REXX)
      DosFreeMem( shv.shvvalue.strptr );
#else
      free( shv.shvvalue.strptr );
#endif
   }
   else
      value = NULL;
   return( value );
}

/*-----------------------------------------------------------------------------
 * Converts a RXSTRING to integer. Return 0 if OK and -1 if error.
 * Assumes a string of decimal digits and does not check for overflow!
 *----------------------------------------------------------------------------*/
int StrToInt

#ifdef HAVE_PROTO
   (RXSTRING *ptr, ULONG *result) 
#else
   (ptr, result) 
   RXSTRING *ptr;
   ULONG    *result; 
#endif

{
   int    i=0;
   char   *p=NULL;
   ULONG  sum=0L;

   p = (char *)ptr->strptr;
   for (i = ptr->strlength; i; i--)
   {
      if (isdigit(*p))
         sum = sum * 10 + (*p++ - '0');
      else
         return -1;
   }
   *result = sum;
   return 0;
}

/*-----------------------------------------------------------------------------
 * Converts a RXSTRING to integer. Return 0 if OK and -1 if error.
 * Assumes a string of decimal digits and does not check for overflow!
 *----------------------------------------------------------------------------*/
int StrToNumber

#ifdef HAVE_PROTO
   (RXSTRING *ptr, LONG *result) 
#else
   (ptr, result) 
   RXSTRING *ptr;
   LONG     *result;
#endif

{
   int    i=0;
   char   *p=NULL;
   LONG   sum=0L;
   int    neg=0;

   p = (char *)ptr->strptr;
   for (i = ptr->strlength; i; i--)
   {
      if (isdigit(*p))
         sum = sum * 10 + (*p - '0');
      else if ( i == ptr->strlength && *p == '-' )
         neg = 1;
      else if ( i == ptr->strlength && *p == '+' )
         ;
      else
         return -1;
      p++;
   }
   if ( neg )
      sum *= -1;
   *result = sum;
   return 0;
}


/*-----------------------------------------------------------------------------
 * Converts a RXSTRING to a boolean. Input can be ON, Yes, 1 or OFF No, 0
 *----------------------------------------------------------------------------*/
int StrToBool

#ifdef HAVE_PROTO
   (RXSTRING *ptr, ULONG *result) 
#else
   (ptr, result) 
   RXSTRING *ptr;
   ULONG    *result; 
#endif

{
   char   *p=(char *)ptr->strptr;
   int    len=ptr->strlength;

   if (memcmp(p,"YES",len) == 0
   ||  memcmp(p,"yes",len) == 0
   ||  memcmp(p,"Y",len) == 0
   ||  memcmp(p,"y",len) == 0
   ||  memcmp(p,"ON",len) == 0
   ||  memcmp(p,"on",len) == 0
   ||  memcmp(p,"1",len) == 0)
   {
      *result = 1;
      return(0);
   }
   if (memcmp(p,"NO",len) == 0
   ||  memcmp(p,"no",len) == 0
   ||  memcmp(p,"N",len) == 0
   ||  memcmp(p,"n",len) == 0
   ||  memcmp(p,"OFF",len) == 0
   ||  memcmp(p,"off",len) == 0
   ||  memcmp(p,"0",len) == 0)
   {
      *result = 0;
      return(0);
   }
   return (-1);
}


/*-----------------------------------------------------------------------------
 * This is called when in VERBOSE mode. It prints function name & arg values.
 *----------------------------------------------------------------------------*/
RxPackageGlobalDataDef *FunctionPrologue

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, PackageInitialiser *RxPackageInitialiser, char *name, ULONG argc, RXSTRING *argv )
#else
   ( RxPackageGlobalData, name, argc, argv )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
   ULONG argc;
   RXSTRING *argv;
#endif

{
   ULONG i = 0L;
   char buf[61];
   RxPackageGlobalDataDef *GlobalData;
   int rc;

   if ( RxPackageGlobalData == NULL )
   {
      GlobalData = InitRxPackage( RxPackageGlobalData, RxPackageInitialiser, &rc );
   }
   else
   {
      GlobalData = RxPackageGlobalData;
   }

   if ( GlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( GlobalData->RxTraceFilePointer, "++\n" );
      (void)fprintf( GlobalData->RxTraceFilePointer, "++ Call %s%s\n", name, argc ? "" : "()" );
      for (i = 0; i < argc; i++) 
      {
         (void)fprintf( GlobalData->RxTraceFilePointer, "++ %3ld: \"%s\"\n",
            i+1,
            MkAsciz( buf, sizeof(buf), (char *)RXSTRPTR( argv[i] ), (int)RXSTRLEN( argv[i] ) ) );
      }
      fflush( GlobalData->RxTraceFilePointer );
   }
   /*
    * This should be replaced with an integer to make things
    * run faster!!
    */
   if ( strcmp( name, GlobalData->FName ) != 0 )
      strcpy( GlobalData->FName, name );

   return GlobalData;
}

/*-----------------------------------------------------------------------------
 * This is called when in VERBOSE mode. It prints function name & return value.
 *----------------------------------------------------------------------------*/
long FunctionEpilogue

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, long rc )
#else
   ( RxPackageGlobalData, name, rc )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
   long rc;
#endif

{
   if ( RxPackageGlobalData == NULL )
   {
      return rc;
   }

   if ( RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer, "++\n" );
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer, "++ Exiting %s() with %ld\n", name, rc );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   return rc;
}

/*-----------------------------------------------------------------------------
 * This is called when in VERBOSE mode. It prints function name & return value.
 *----------------------------------------------------------------------------*/
void FunctionTrace

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, ... )
#else
   ( RxPackageGlobalData, name )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
#endif
{
   va_list argptr;
   char *fmt=NULL;

   if ( RxPackageGlobalData == NULL )
   {
      return;
   }

   if ( RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ">>\n" );
      va_start( argptr, name );
      fmt = va_arg( argptr, char *);
      if (fmt != NULL)
      {
         (void)fprintf( RxPackageGlobalData->RxTraceFilePointer, ">> " );
         (void)vfprintf( RxPackageGlobalData->RxTraceFilePointer, fmt, argptr );
      }
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ")\n");
      fflush( RxPackageGlobalData->RxTraceFilePointer );
      va_end( argptr );
   }
   return;
}

/*-----------------------------------------------------------------------------
 * This is called when run_flags & MODE_INTERNAL is true
 *----------------------------------------------------------------------------*/
void InternalTrace

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name, ...)
#else
   ( RxPackageGlobalData, name )
    RxPackageGlobalDataDef *RxPackageGlobalData;
    char *name;
#endif

{
   va_list argptr;
   char *fmt=NULL;

   if ( RxPackageGlobalData != NULL
   &&   RxPackageGlobalData->RxRunFlags & MODE_INTERNAL )
   {
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ">>>> Call %s(", name );
      va_start( argptr, name );
      fmt = va_arg( argptr, char *);
      if (fmt != NULL)
      {
         (void)vfprintf( RxPackageGlobalData->RxTraceFilePointer, fmt, argptr );
      }
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ")\n");
      fflush( RxPackageGlobalData->RxTraceFilePointer );
      va_end( argptr );
   }
   return;
}
void RxDisplayError

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RFH_ARG0_TYPE name, ...)
#else
   ( RxPackageGlobalData, name )
    RxPackageGlobalDataDef *RxPackageGlobalData;
    char *name;
#endif

{
   va_list argptr;
   char *fmt=NULL;

   if ( RxPackageGlobalData != NULL )
   {
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ">>>> Calling %s(", name );
      va_start( argptr, name );
      fmt = va_arg( argptr, char *);
      if (fmt != NULL)
      {
         (void)vfprintf( RxPackageGlobalData->RxTraceFilePointer, fmt, argptr );
      }
      (void) fprintf( RxPackageGlobalData->RxTraceFilePointer, ")\n");
      fflush( RxPackageGlobalData->RxTraceFilePointer );
      va_end( argptr );
   }
   return;
}


/*-----------------------------------------------------------------------------
 * This function registers the default subcommand handler...
 *----------------------------------------------------------------------------*/
int RegisterRxSubcom

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RexxSubcomHandler ptr)
#else
   ( RxPackageGlobalData, ptr )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RexxSubcomHandler ptr;
#endif

{
   ULONG rc=0L;

   InternalTrace( RxPackageGlobalData, "RegisterRxSubcom", NULL );

#if defined(USE_REXX6000)
   rc = RexxRegisterSubcom( (RRSE_ARG0_TYPE)RXPACKAGENAME,
                            (RRSE_ARG1_TYPE)(ptr) ? ptr : RxSubcomHandler,
                            (RRSE_ARG2_TYPE)NULL );
#else
   rc = RexxRegisterSubcomExe( (RRSE_ARG0_TYPE)RXPACKAGENAME,
                               (RRSE_ARG1_TYPE)(ptr) ? ptr : RxSubcomHandler,
                               (RRSE_ARG2_TYPE)NULL );
#endif
   if ( rc != RXSUBCOM_OK )
      return 1;
   return 0;
}

/*-----------------------------------------------------------------------------
 * This function registers the external functions...
 *----------------------------------------------------------------------------*/
int RegisterRxFunctions

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RexxFunction *RxPackageFunctions )
#else
   ( RxPackageGlobalData,  RxPackageFunctions )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RexxFunction *RxPackageFunctions;
#endif

{
   RexxFunction *func=NULL;
   ULONG rc=0L;

   InternalTrace( RxPackageGlobalData, "RegisterRxFunctions", NULL );

   for ( func = RxPackageFunctions; func->InternalName; func++ )
   {
#if defined(DYNAMIC_LIBRARY)
# if !defined(USE_REXX6000)
      if (func->DllLoad)
      {
         rc = RexxRegisterFunctionDll( func->ExternalName,
              RXPACKAGENAME,
              func->InternalName );
         InternalTrace( RxPackageGlobalData, "RegisterRxFunctions","%s-%d: Registered (DLL) %s with rc = %ld\n",__FILE__,__LINE__,func->ExternalName,rc);
      }
# endif
#else
# if defined(WIN32) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
      (void)RexxDeregisterFunction( func->ExternalName );
# endif
# if defined(USE_REXX6000)
      rc = RexxRegisterFunction( func->ExternalName, func->EntryPoint, NULL );
      InternalTrace( RxPackageGlobalData, "RegisterRxFunctions","%s-%d: Registered (EXE) %s with rc = %d\n",__FILE__,__LINE__,func->ExternalName,rc);
# else
      rc = RexxRegisterFunctionExe( func->ExternalName, func->EntryPoint );
      InternalTrace( RxPackageGlobalData, "RegisterRxFunctions","%s-%d: Registered (EXE) %s with rc = %d\n",__FILE__,__LINE__,func->ExternalName,rc);
# endif
#endif
      if (rc != RXFUNC_OK
#ifdef RXFUNC_DUP
      &&  rc != RXFUNC_DUP /* bug in Object Rexx for Linux */
#else
      &&  rc != RXFUNC_DEFINED
#endif
      &&  rc != RXFUNC_NOTREG)
         return 1;
   }
#if !defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
   if ( RexxRegisterExitExe( ( RREE_ARG0_TYPE )RXPACKAGENAME,
                             ( RREE_ARG1_TYPE )RxExitHandlerForSayTraceRedirection,
                             ( RREE_ARG2_TYPE )NULL) != RXEXIT_OK )
      return 1;
#endif
   return 0 ;
}

/*-----------------------------------------------------------------------------
 * This function queries if the supplied external function has been registered
 *----------------------------------------------------------------------------*/
int QueryRxFunction

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *funcname )
#else
   ( RxPackageGlobalData, funcname )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *funcname;
#endif

{
   InternalTrace( RxPackageGlobalData, "QueryRxFunction", "%s", funcname );

#if defined(USE_REXX6000)
   if ( RexxQueryFunction( funcname, NULL ) == RXFUNC_OK )
#else
   if ( RexxQueryFunction( funcname ) == RXFUNC_OK )
#endif
   {
      DEBUGDUMP(fprintf(stderr,"%s-%d: Query function %s - found\n",__FILE__,__LINE__,funcname);)
      return 1;
   }
   DEBUGDUMP(fprintf(stderr,"%s-%d: Query function %s - not found\n",__FILE__,__LINE__,funcname);)
   return 0;
}

/*-----------------------------------------------------------------------------
 * This function deregisters the external functions...
 *----------------------------------------------------------------------------*/
int DeregisterRxFunctions

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RexxFunction *RxPackageFunctions, int verbose )
#else
   ( RxPackageGlobalData, RxPackageFunctions, verbose )
   RexxFunction *RxPackageFunctions;
   RxPackageGlobalDataDef *RxPackageGlobalData;
   int verbose;
#endif

{
   RexxFunction *func=NULL;
   int rc=0;

   InternalTrace( RxPackageGlobalData, "DeregisterRxFunctions", "%d", verbose );

   for ( func = RxPackageFunctions; func->InternalName; func++ )
   {
      assert( func->ExternalName );
      rc = RexxDeregisterFunction( func->ExternalName );
      if ( verbose )
         fprintf(stderr,"Deregistering...%s - %d\n",func->ExternalName,rc);
      DEBUGDUMP(fprintf(stderr,"%s-%d: Deregistered %s with rc %d\n",__FILE__,__LINE__,func->ExternalName,rc);)
   }
#if !defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
   RexxDeregisterExit( RXPACKAGENAME, NULL );
#endif
   return 0 ;
}

/*-----------------------------------------------------------------------------
 * This function is called to initialise the package
 *----------------------------------------------------------------------------*/
RxPackageGlobalDataDef *InitRxPackage

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *MyGlob, PackageInitialiser *ptr, int *rc )
#else
   ( MyGlob, ptr, rc )
   RxPackageGlobalDataDef *MyGlob;
   PackageInitialiser *ptr;
   int *rc;
#endif

{
   char *env;
   RxPackageGlobalDataDef *RxPackageGlobalData;

   DEBUGDUMP(fprintf(stderr,"%s-%d: Start of InitRxPackage\n",__FILE__,__LINE__);)
   if ( MyGlob )
   {
      RxPackageGlobalData = MyGlob;
   }
   else
   {
      if ( ( RxPackageGlobalData = ( RxPackageGlobalDataDef *)malloc( sizeof( RxPackageGlobalDataDef ) ) ) == NULL )
      {
         fprintf( stderr, "Unable to allocate memory for Global Data\n" );
         *rc = 1;
         return NULL;
      }
      memset( RxPackageGlobalData, 0, sizeof( RxPackageGlobalDataDef ) );
      (void)RxSetTraceFile( RxPackageGlobalData, "stderr" );
   }
   if ( (env = getenv(RXPACKAGE_DEBUG_VAR)) != NULL )
   {
      RxPackageGlobalData->RxRunFlags |= atoi(env);
   }
   /* 
    * Call any package-specific startup code here
    */
   if ( ptr )
   {
      *rc = (*ptr)( );
   }
   DEBUGDUMP(fprintf(stderr,"%s-%d: End of InitRxPackage with rc = %ld\n",__FILE__,__LINE__,rc);)
   return RxPackageGlobalData;
}


/*-----------------------------------------------------------------------------
 * This function is called to terminate all activity with the package.
 *----------------------------------------------------------------------------*/
int TermRxPackage

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, PackageTerminator *ptr, RexxFunction *RxPackageFunctions, char *progname, int deregfunc )
#else
   ( RxPackageGlobalData, ptr, RxPackageFunctions, progname, deregfunc )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   PackageTerminator *ptr;
   RexxFunction *RxPackageFunctions;
   char *progname;
   int deregfunc;
#endif
{
   int rc=0;

   InternalTrace( RxPackageGlobalData, "TermRxPackage", "\"%s\",%d", progname, deregfunc );

   DEBUGDUMP(fprintf(stderr,"%s-%d: Start of TermRxPackage\n",__FILE__,__LINE__);)
   /* 
    * De-register all REXX/SQL functions only 
    * if DEBUG value = 99
    * DO NOT DO THIS FOR DYNAMIC LIBRARY
    * AS IT WILL DEREGISTER FOR ALL PROCESSES
    * NOT JUST THE CURRENT ONE.               
    */
   if (deregfunc)
   {
      if ( ( rc = DeregisterRxFunctions( RxPackageGlobalData, RxPackageFunctions, 0 ) ) != 0 )
         return (int)FunctionEpilogue( RxPackageGlobalData, "TermRxPackage", (long)rc );
   }
   /* 
    * Call any package-specific termination code here
    */
   if ( ptr )
   {
      if ( ( rc = (*ptr)( ) ) != 0 )
         return (int)FunctionEpilogue( RxPackageGlobalData, "TermRxPackage", (long)rc );
   }
#if defined(USE_REXX6000)
   rc = RexxDeregisterSubcom( RDS_ARG0_TYPE)RXPACKAGENAME );
#else
   rc = RexxDeregisterSubcom( (RDS_ARG0_TYPE)RXPACKAGENAME,
                              (RDS_ARG1_TYPE)NULL );
#endif

#if !defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
   RexxDeregisterExit( ( RDE_ARG0_TYPE )RXPACKAGENAME,
                       ( RDE_ARG1_TYPE )NULL );
#endif
   if ( RxPackageGlobalData->RxTraceFilePointer != stdin
   &&   RxPackageGlobalData->RxTraceFilePointer != stderr )
      fclose( RxPackageGlobalData->RxTraceFilePointer );

   free( RxPackageGlobalData ); /* TODO - only free if requested */
   DEBUGDUMP(fprintf(stderr,"%s-%d: End of TermRxPackage with rc = 0\n",__FILE__,__LINE__);)
   return (int)FunctionEpilogue( RxPackageGlobalData, "TermRxPackage", (long)0 );
}


/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxSetTraceFile

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, char *name )
#else
   ( RxPackageGlobalData, name )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   char *name;
#endif

{
   FILE *fp = NULL;

   InternalTrace( RxPackageGlobalData, "RxSetTraceFile", "%s", name );

   if ( strcmp( "stdin", name ) == 0 )
   {
      RxPackageGlobalData->RxTraceFilePointer = stdin;
      strcpy( RxPackageGlobalData->RxTraceFileName, "stdin" );
   }
   else
   {
      if ( strcmp( "stderr", name ) == 0 )
      {
         RxPackageGlobalData->RxTraceFilePointer = stderr;
         strcpy( RxPackageGlobalData->RxTraceFileName, "stderr" );
      }
      else
      {
         if ( RxPackageGlobalData->RxTraceFilePointer != NULL )
            fclose( RxPackageGlobalData->RxTraceFilePointer );
         fp = fopen( name, "w" );
         if ( fp == NULL )
         {
            (void)fprintf( stderr, "ERROR: Could not open trace file: %s for writing\n", name );
            return( 1 );
         }
         RxPackageGlobalData->RxTraceFilePointer = fp;
         strcpy( RxPackageGlobalData->RxTraceFileName, name );
      }
   }
   return( 0 );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
char *RxGetTraceFile

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData )
#else
   ( RxPackageGlobalData )
   RxPackageGlobalDataDef *RxPackageGlobalData;
#endif

{
   InternalTrace( RxPackageGlobalData, "RxGetTraceFile", NULL );
   return ( RxPackageGlobalData->RxTraceFileName );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
void RxSetRunFlags

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, int flags )
#else
   ( RxPackageGlobalData, flags )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   int flags;
#endif

{
   InternalTrace( RxPackageGlobalData, "RxSetRunFlags", "%d", flags );
   RxPackageGlobalData->RxRunFlags = flags;
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxGetRunFlags

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData )
#else
   ( RxPackageGlobalData )
   RxPackageGlobalDataDef *RxPackageGlobalData;
#endif

{  
   InternalTrace( RxPackageGlobalData, "RxGetRunFlags", NULL );
   return ( RxPackageGlobalData->RxRunFlags );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxReturn

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr )
#else
   ( RxPackageGlobalData, retstr )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
#endif
{
   InternalTrace( RxPackageGlobalData, "RxReturn" );

   if ( RxPackageGlobalData
   &&   RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
          "++ Exit %s with value \"%s\"\n",
          RxPackageGlobalData->FName,
          retstr->strptr );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   return( 0 );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxReturnNumber

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr, long num )
#else
   ( RxPackageGlobalData, retstr, num )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
   long num;
#endif
{
   InternalTrace( RxPackageGlobalData, "RxReturnNumber", "%x,%d", retstr, num );

   sprintf( (char *)retstr->strptr, "%ld", num );
   retstr->strlength = strlen( (char *)retstr->strptr );
   if ( RxPackageGlobalData
   &&   RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
         "++ Exit %s with value \"%ld\"\n",
         RxPackageGlobalData->FName,
         num );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   return( 0 );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxReturnDouble

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr, double num )
#else
   ( RxPackageGlobalData, retstr, num )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
   long num;
#endif
{
   InternalTrace( RxPackageGlobalData, "RxReturnDouble", "%x,%f", retstr, num );

   sprintf( (char *)retstr->strptr, "%f", num );
   retstr->strlength = strlen( (char *)retstr->strptr );
   if ( RxPackageGlobalData
   &&   RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
         "++ Exit %s with value \"%f\"\n",
         RxPackageGlobalData->FName,
         num );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   return( 0 );
}


/*-----------------------------------------------------------------------------
 * This function returns a numeric representation of a pointer.
 * If NULL, returns an empty string.
 *----------------------------------------------------------------------------*/
int RxReturnPointer

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr, void *ptr )
#else
   ( RxPackageGlobalData, retstr, ptr )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
   void *ptr;
#endif
{
   InternalTrace( RxPackageGlobalData, "RxReturnPointer", "%x,%x", retstr, ptr );

   if ( ptr )
   {
      sprintf( (char *)retstr->strptr, "%ld", (long)ptr );
      retstr->strlength = strlen( (char *)retstr->strptr );
   }
   else
   {
      strcpy( (char *)retstr->strptr, "" );
      retstr->strlength = 0;
   }
   if ( RxPackageGlobalData
   &&   RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
         "++ Exit %s with value \"%s\"\n",
         RxPackageGlobalData->FName,
         retstr->strptr );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   return( 0 );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxReturnString

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr, char *str )
#else
   ( RxPackageGlobalData, retstr, str )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
   char *str;
#endif
{
   return( RxReturnStringAndFree( RxPackageGlobalData, retstr, str, 0 ) );
}

/*-----------------------------------------------------------------------------
 * This function 
 *----------------------------------------------------------------------------*/
int RxReturnStringAndFree

#ifdef HAVE_PROTO
   ( RxPackageGlobalDataDef *RxPackageGlobalData, RXSTRING *retstr, char *str, int freeit )
#else
   ( RxPackageGlobalData, retstr, str )
   RxPackageGlobalDataDef *RxPackageGlobalData;
   RXSTRING *retstr;
   char *str;
#endif
{
   int len = strlen( str );
   char *ret = NULL;

   InternalTrace( RxPackageGlobalData, "RxReturnStringAndFree", "%x,\"%s\" Length: %d Free: %d", retstr, str, len, freeit );

   if ( len > RXAUTOBUFLEN )
   {
#if defined(REXXFREEMEMORY)
      ret = (char *)RexxAllocateMemory( len + 1 );
#elif defined(WIN32) && ( defined(USE_WINREXX) || defined(USE_QUERCUS) || defined(USE_OREXX) )
      ret = ( char * )GlobalLock( GlobalAlloc ( GMEM_FIXED, len + 1 ) );
#elif defined(USE_OS2REXX)
      if ( ( BOOL )DosAllocMem( (void **)&ret, len + 1, fPERM|PAG_COMMIT ) )
         ret = ( char * )NULL;
#else
      ret = ( char * )malloc( len + 1 );
#endif
      if ( ret == ( char * )NULL )
      {
         fprintf( stderr, "Unable to allocate %d bytes for return string \"%s\"\n", len, str );
         return( 1 );
      }
      retstr->strptr = (RXSTRING_STRPTR_TYPE)ret;
   }
   memcpy( retstr->strptr, str, len );
   retstr->strlength = len;
   if ( RxPackageGlobalData
   &&   RxPackageGlobalData->RxRunFlags & MODE_VERBOSE )
   {
      (void)fprintf( RxPackageGlobalData->RxTraceFilePointer,
         "++ Exit %s with value \"%s\"\n",
         RxPackageGlobalData->FName,
         str );
      fflush( RxPackageGlobalData->RxTraceFilePointer );
   }
   if ( freeit ) free( str );
   return( 0 );
}

#if !defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
/*-----------------------------------------------------------------------------
 * This function
 *----------------------------------------------------------------------------*/
static REH_RETURN_TYPE RxExitHandlerForSayTraceRedirection

#if defined(HAVE_PROTO)
   ( REH_ARG0_TYPE ExitNumber, REH_ARG1_TYPE Subfunction, REH_ARG2_TYPE ParmBlock )
#else
   ( ExitNumber, Subfunction, ParmBlock )
   REH_ARG0_TYPE ExitNumber;    /* code defining the exit function    */
   REH_ARG1_TYPE Subfunction;   /* code defining the exit subfunction */
   REH_ARG2_TYPE ParmBlock;     /* function dependent control block   */
#endif
{
   long i = 0;
   int rc = 0;

   switch( Subfunction )
   {
      case RXSIOSAY:
      {
         RXSIOSAY_PARM *say_parm = (RXSIOSAY_PARM *)ParmBlock;
         if ( say_parm->rxsio_string.strptr != NULL )
         {
            for( i = 0; i < (long)say_parm->rxsio_string.strlength; i++ )
               fputc( ( char )say_parm->rxsio_string.strptr[i], 
                  stdout );
         }
         fputc( '\n', stdout );
         rc = RXEXIT_HANDLED;
         break;
      }
      case RXSIOTRC:
      {
         RXSIOTRC_PARM *trc_parm = (RXSIOTRC_PARM *)ParmBlock;
         if ( trc_parm->rxsio_string.strptr != NULL )
         {
            for( i = 0; i < (long)trc_parm->rxsio_string.strlength; i++ )
               fputc( ( char )trc_parm->rxsio_string.strptr[i], 
                  stderr );
         }
         fputc( '\n', stderr );
         rc = RXEXIT_HANDLED;
         break;
      }
      case RXSIOTRD:
      {
         RXSIOTRD_PARM *trd_parm = (RXSIOTRD_PARM *)ParmBlock;
         int i = 0, ch = 0;
         do
         {
            if ( i < 256 )
               trd_parm->rxsiotrd_retc.strptr[i++] = ch = getc( stdin ) ;
         } while( ch != '\012' && (ch != EOF ) ) ;
         trd_parm->rxsiotrd_retc.strlength = i - 1;
         rc = RXEXIT_HANDLED;
         break;
      }
      case RXSIODTR:
      {
         RXSIODTR_PARM *dtr_parm = (RXSIODTR_PARM *)ParmBlock;
         int i = 0, ch = 0;
         do
         {
            if ( i < 256 )
               dtr_parm->rxsiodtr_retc.strptr[i++] = ch = getc( stdin ) ;
         } while( ch != '\012' && (ch != EOF ) ) ;
         dtr_parm->rxsiodtr_retc.strlength = i - 1;
         rc = RXEXIT_HANDLED;
         break;
      }
      default:
         rc = RXEXIT_NOT_HANDLED;
         break;
   }
   return rc;
}
#endif

/***********************************************************************/
static RSH_RETURN_TYPE RxSubcomHandler

#if defined(HAVE_PROTO)
   (
   RSH_ARG0_TYPE Command,    /* Command string passed from the caller    */
   RSH_ARG1_TYPE Flags,      /* pointer to short for return of flags     */
   RSH_ARG2_TYPE Retstr)     /* pointer to RXSTRING for RC return        */
#else
   ( Command, Flags, Retstr )
   RSH_ARG0_TYPE Command;    /* Command string passed from the caller    */
   RSH_ARG1_TYPE Flags;      /* pointer to short for return of flags     */
   RSH_ARG2_TYPE Retstr;     /* pointer to RXSTRING for RC return        */
#endif
/***********************************************************************/
{
   RSH_RETURN_TYPE rcode=0;
   int rc;
   char *buf;

   buf = malloc( Command->strlength + 1 );
   if ( buf == NULL )
   {
      *Flags = RXSUBCOM_ERROR;             /* raise an error condition   */
      sprintf(Retstr->strptr, "%d", rc);   /* format return code string  */
                                           /* and set the correct length */
      Retstr->strlength = strlen(Retstr->strptr);
   }
   else
   {
      memcpy( buf, Command->strptr, Command->strlength );
      buf[Command->strlength] = '\0';
      rc = system( buf );
      free( buf );
      if (rc < 0)
         *Flags = RXSUBCOM_ERROR;             /* raise an error condition   */
      else
         *Flags = RXSUBCOM_OK;                /* not found is not an error  */
   
      sprintf(Retstr->strptr, "%d", rc); /* format return code string  */
                                              /* and set the correct length */
      Retstr->strlength = strlen(Retstr->strptr);
   }
   return rcode;                              /* processing completed       */
}

/*
 * Both WinRexx and Personal Rexx load and unload this DLL before and
 * after each call to an external function. If this DLL is not resident
 * a new copy of the DLL is loaded together with its data.  What happens
 * then is any data set by one call to an external function is lost
 * and there is no persistence of data.  This code ensures that when the
 * DLL starts, it stays resident, and when the DLL finishes it is
 * unloaded.
 */
#if defined(DYNAMIC_LIBRARY) && (defined(USE_WINREXX) || defined(USE_QUERCUS))
BOOL WINAPI DllMain( HINSTANCE hDLL, DWORD dwReason, LPVOID pReserved)
{
   switch( dwReason)
   {
      case DLL_PROCESS_ATTACH:
         LoadLibrary( RXPACKAGENAME );
         break;
      case DLL_PROCESS_DETACH:
         FreeLibrary( GetModuleHandle( RXPACKAGENAME ) );
         break;
      case DLL_THREAD_ATTACH:
         break;
      case DLL_THREAD_DETACH:
         break;
   }
   return(TRUE);
}
#endif

