/***************************************************************************
                          ncdf_cl.cpp  -  NetCDF GDL library function
                             -------------------
    begin                : March 24 2004
    copyright            : (C) 2004 by Christopher Lee
    email                : leec_gdl@publius.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#else
// default: assume we have netCDF
#define USE_NETCDF 1
#endif

#ifdef USE_NETCDF

#include "includefirst.hpp"

#include <string>
#include <fstream>
#include <memory>
#include <gsl/gsl_sys.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_sf.h>

#include "datatypes.hpp"
#include "math_utl.hpp"
#include "envt.hpp"
#include "dpro.hpp"
#include "dinterpreter.hpp"
#include "ncdf_cl.hpp"
#include "terminfo.hpp"
#include "typedefs.hpp"



#define GDL_DEBUG
//#undef GDL_DEBUG

namespace lib {

  using namespace std;
  using namespace antlr;
  bool ncdf_verbose=true;
  

  DStringGDL ncdf_gdl_typename(nc_type vartype)
  {
    switch (vartype)
      {
      case NC_BYTE:  return DStringGDL("BYTE");//8 bit
      case NC_CHAR:  return DStringGDL("CHAR");//8 bit as string
      case NC_SHORT: return DStringGDL("INT");//16 bit
      case NC_INT:   return DStringGDL("LONG");//32 bit
      case NC_FLOAT: return DStringGDL("FLOAT");//32 bit float
      case NC_DOUBLE:return DStringGDL("DOUBLE");//64 bit double
      }
    return DStringGDL("UNKNOWN");
  }

  string _ncdf_itos(int i)
  {
    int mema=3;
    char *n=new char(mema);
    while (snprintf(n, sizeof n, "%d", i) >= sizeof n)
      {			delete n;mema++; n=new char(mema);   }
    string s=n;
    delete n;
    return s;
  }
  
  void ncdf_handle_error(EnvT *e, int status, const char *function)
  {
    if(status != NC_NOERR)
      {
	string error;
	error=function;
	error+=": ";
	DString s;
	DInt i;
	

	if(status==NC_EBADID)	/* Not a netcdf id */
	  {
	    e->AssureScalarPar<DIntGDL>(0,i);
	    error+=_ncdf_itos(i);
	    error+=" is not a valid cdfid";
	    
	    error+="(NC_ERROR=-33)";
	  }
	else if(status==NC_ENFILE)	/* Too many netcdfs open */
	  {
	    error+="Too many NetCDF files open. (NC_ERROR=-34)";
	  }
	else if(status==NC_EEXIST) 	/* netcdf file exists && NC_NOCLOBBER */
	  {
	    e->AssureScalarPar<DStringGDL>(0, s);
	    error+="Unable to create the file, "  + s + ". ";
	    error+="(NC_ERROR=-35)";
	  }
	else if(status==NC_EINVAL) 	/* Invalid Argument */
	  {
	    error+="(NC_ERROR=-36)";
	  }
	else if(status==NC_EPERM) 	/* Write to read only */
	  {
	    error+="Write permission not enabled.";
	    
	    error+="(NC_ERROR=-37)";
	  }
      	else if(status==NC_ENOTINDEFINE) /* Operation not allowed in data mode */
	  {
	    error+=" Unable to define variable, not in define mode.";
	    
	    error+="(NC_ERROR=-38)";
	  }
	else if(status== NC_EINDEFINE) 	/* Operation not allowed in define mode */
	  {
	    error+=" Cannot acces data in DEFINE mode.";
	    
	    error+="(NC_ERROR=-39)";
	  }
	else if(status==NC_EINVALCOORDS) /* Index exceeds dimension bound */
	  {
	    //this error should never be triggered
	    error+="GDL INTERNAL ERROR, PLEASE REPORT TO CODE MAINTAINER";
	    
	    error+="(NC_ERROR=-40)";
	  }
	else if(status==NC_EMAXDIMS) 	/* NC_MAX_DIMS exceeded */
	  {
	    error+="Unable to define variable, maximum number of attributes exceeded";
	    error+="(NC_ERROR=-41)";
	  }
	else if(status==NC_ENAMEINUSE) 	/* String match to name in use */
	  {
	    error+="(NC_ERROR=-42)";
	  }
      	else if(status==NC_ENOTATT) /* Attribute not found */
	  {
	    error+=" Attribute enquiry failed. ";
	    
	    error+="(NC_ERROR=-43)";
	  }
	else if(status==NC_EMAXATTS) 	/* NC_MAX_ATTRS exceeded */
	  {
	    error+="Attribute write failed, maximum number of attributes exceeded";
	    error+="(NC_ERROR=-44)";
	  }
      	else if(status==NC_EBADTYPE) 	/* Not a netcdf data type */    
	  {
	    error+="(NC_ERROR=-45)";
	  }
	else if(status==NC_EBADDIM)  	/* Invalid dimension id or name */  
	  {
	    e->AssureScalarPar<DIntGDL>(0,i);
	    error+="No Dimension with id = ";
	    error+=_ncdf_itos(i);
	    error+=" found. ";
	    
	    error+="(NC_ERROR=-46)";
	  }
	else if(status==NC_EUNLIMPOS) 	/* NC_UNLIMITED in the wrong index */ 
	  {
	    error+="(NC_ERROR=-47)";
	  }
      	else if(status==NC_EMAXVARS) 	/* NC_MAX_VARS exceeded */
	  {
	    error+="Unable to define variable, maximum number of attributes exceeded";
	    error+="(NC_ERROR=-48)";
	  }
	else if(status==NC_ENOTVAR) 	/* Variable not found */
	  {
	    e->AssureScalarPar<DIntGDL>(0,i);
	    error+="Variable enquiry failed, ";
	    error+=_ncdf_itos(i);	    
	    error+=" is not a valid variable id.";
	    error+="(NC_ERROR=-49)";
	  }
      	else if(status==NC_EGLOBAL) 	/* Action prohibited on NC_GLOBAL varid */
	  {
	    error+="(NC_ERROR=-50)";
	  }
	else if(status==NC_ENOTNC)  	/* Not a netcdf file */
	  {
	    e->AssureScalarPar<DStringGDL>(0, s);
	    error+=" Unable to open the file \""+s+ "\". ";
	    error+="(NC_ERROR=-51)";
	  }
	else if(status==NC_ESTS)  	/* In Fortran, string too short */
	  {
	    error+="(NC_ERROR=-52)";
	  }
      	else if(status==NC_EMAXNAME)  	/* NC_MAX_NAME exceeded */
	  {
	    error+="(NC_ERROR=-53)";
	  }
	else if(status==NC_EUNLIMIT)  	/* NC_UNLIMITED size already in use */
	  {
	    error+="Unable to create dimension, NC_UNLIMITED dimension alread in use. ";
	    
	    error+="(NC_ERROR=-54)";
	  }
	else if(status==NC_ENORECVARS) 	/* nc_rec op when there are no record vars */ 
	  {
	    error+="(NC_ERROR=-55)";
	  }
	else if(status==NC_ECHAR)   	/* Attempt to convert between text & numbers */
	  {
	    error+="(NC_ERROR=-56)";
	  }
	else if(status==NC_EEDGE)   	/* Edge+start exceeds dimension bound */
	  {
	    //this error should never be triggered
	    error+="GDL INTERNAL ERROR, PLEASE REPORT TO CODE MAINTAINER";

	    error+="(NC_ERROR=-57)";
	  }
	else if(status==NC_ESTRIDE) 	/* Illegal stride */
	  {
	    error+="(NC_ERROR=-58)";
	  }
	else if(status==NC_EBADNAME)	/* Attribute or variable name
                                        contains illegal characters */  
	  {
    
	    error+="(NC_ERROR=-59)";
	  }
	else if(status==NC_ERANGE)  	/* Math result not representable */ 
	  {
	    error+="(NC_ERROR=-60)";
	  }

	else if(status==NC_ENOMEM)	/* Memory allocation (malloc) failure */
	  {
	    error+="(NC_ERROR=-61)";
	  }
	else if(status==2)
	  {
	    e->AssureScalarPar<DStringGDL>(0, s);
	    error+=" Unable to open the file \""+s+ "\". (NC_ERROR = 2)";    
	  }
	else 
	  {
	  /*unknown error*/
	    int mema=3;
	    char *n=new char(mema);
	    while (snprintf(n, sizeof n, "%d", status) >= sizeof n)
	      {			delete n;mema++; n=new char(mema);   }

	    error+=nc_strerror(status);
	    error+=" (NC_ERROR=";
	    error+=n;
	    delete n;
	    error+=")";
	    
	  }
	

	throw GDLException(e->CallingNode(),
			   error);

      }

  }

  //open the ncdf file of given filename
  BaseGDL * ncdf_open(EnvT * e)
  {
    size_t nParam=e->NParam(1);
    if(nParam != 1) throw GDLException(e->CallingNode(),
				       "NCDF_OPEN: Wrong number of arguments.");
    
    
    DString s;
    e->AssureScalarPar<DStringGDL>(0, s);

    int cdfid,status;
    if(e->KeywordSet("WRITE") &&!e->KeywordSet("NOWRITE"))
      {
	status=nc_open(s.c_str(),
		       NC_WRITE,
		       &cdfid);
      } else {
	status=nc_open(s.c_str(),
		       NC_NOWRITE,
		       &cdfid);
      }

	ncdf_handle_error(e,status,"NCDF_OPEN");

    return new DIntGDL(cdfid);


  }


  //close the NetCDF file
  void ncdf_close(EnvT* e)
  {
    size_t nParam=e->NParam(1);
    if(nParam != 1) throw GDLException(e->CallingNode(),
				       "NCDF_CLOSE: Wrong number of arguments.");
    BaseGDL* n=e->GetParDefined(0);

    DIntGDL* cdfid=static_cast<DIntGDL*>(n);
    DInt in;
    e->AssureScalarPar<DIntGDL>(0, in);
    int status;

    //try closing the number
    status=nc_close((*cdfid)[0]);

      //handle the error
    ncdf_handle_error(e,status,"NCDF_CLOSE");

  }


  //ncdf inquire, returns the struct {NDIMS: 0L,NVARS:0L,NGATTS:0L, RECDIM:0L}
  BaseGDL* ncdf_inquire(EnvT* e)
  {
    size_t nParam=e->NParam(1);
   
    int status, ndims,nvars,ngatts,unlimdimid;
    BaseGDL* n=e->GetParDefined(0);


    DIntGDL* cdfid=static_cast<DIntGDL*>(n);
    DInt in;
    e->AssureScalarPar<DIntGDL>(0, in);    

    status = nc_inq((*cdfid)[0], &ndims, &nvars, &ngatts, &unlimdimid);

    ncdf_handle_error(e,status,"NCDF_INQUIRE");

    DStructDesc* ncdf_inq=new DStructDesc( "NCDF_INQ");
    SpDLong aLong;
    ncdf_inq->AddTag("NDIMS", &aLong);
    ncdf_inq->AddTag("NVARS", &aLong);
    ncdf_inq->AddTag("NGATTS",  &aLong);
    ncdf_inq->AddTag("RECDIM",  &aLong);

    structList.push_back(ncdf_inq);
    
    DStructGDL* inq=new DStructGDL( "NCDF_INQ");
    inq->InitTag("NDIMS",DLongGDL(ndims));
    inq->InitTag("NVARS",DLongGDL(nvars));
    inq->InitTag("NGATTS",DLongGDL(ngatts));
    inq->InitTag("RECDIM",DLongGDL(unlimdimid));
    
    return inq;

  }


  //Create the ncdf file of given filename
  BaseGDL * ncdf_create(EnvT * e)
  {
    size_t nParam=e->NParam(1);
    if(nParam != 1) throw GDLException(e->CallingNode(),
				       "NCDF_CREATE: Wrong number of arguments.");

    
    DString s;
    e->AssureScalarPar<DStringGDL>(0, s);


    int cdfid,status;
    if(e->KeywordSet("CLOBBER") &&!e->KeywordSet("NOCLOBBER"))
      {
	status=nc_create(s.c_str(),
		       NC_CLOBBER,
		       &cdfid);
      } else {
	status=nc_create(s.c_str(),
		       NC_NOCLOBBER,
		       &cdfid);
      }

      ncdf_handle_error(e,status,"NCDF_CREATE");

    return new DIntGDL(cdfid);


  }

  void ncdf_control(EnvT* e)
  {
    /*keywords
 0   ABORT = restores or deletes file
 1   ENDEF = ends define mode, starts data mode
 2   FILL = fills the netcdf file with certain values
 3   NOFILL = opposite of FILL
 4   VERBOSE = verbose error messages, hmm
 5   NOVERBOSE = opposite of verbose
 6   OLDFILL=variable ->result of last fill
 7   REDEF = puts file into define mode
 8   SYNC = update the file on disk
    */
    size_t nParam=e->NParam(1);
    int status,omode    ;

    BaseGDL* n=e->GetParDefined(0);
    DIntGDL* cdfid=static_cast<DIntGDL*>(n);
    DInt in;
    e->AssureScalarPar<DIntGDL>(0, in);

    int total=e->KeywordSet(0)+      e->KeywordSet(1)+
      e->KeywordSet(2)+      e->KeywordSet(3)+
      e->KeywordSet(4)+      e->KeywordSet(5)+
      e->KeywordSet(7)+      e->KeywordSet(8);
    
    if(total != 1) throw GDLException(e->CallingNode(),
				      "NCDF_CONTROL: Error message here(too many keywords).");


    status=NC_NOERR;
    if(e->KeywordSet(0))//ABORT
	status=nc_abort((*cdfid)[0]);
    else if(e->KeywordSet(1))//ENDEF
	status=nc_enddef((*cdfid)[0]);
    else if(e->KeywordSet(2))//FILL 
	status=nc_set_fill((*cdfid)[0],NC_FILL,&omode);
    else if(e->KeywordSet(3))//NOFILL
      status=nc_set_fill((*cdfid)[0],NC_NOFILL,&omode);
    else if(e->KeywordSet(4))//VERBOSE
      ncdf_verbose=true;
    else if(e->KeywordSet(5))//NOVERBOSE
      ncdf_verbose=false;
    else if(e->KeywordSet(7))//REDEF
	status=nc_redef((*cdfid)[0]);
    else if(e->KeywordSet(8))//SYNC
	status=nc_sync((*cdfid)[0]);
    
    if(e->KeywordSet(7) && status==NC_EPERM)
      throw GDLException(e->CallingNode(),"NCDF_CONTROL: Attempt to reenter define mode (REDEF) failed, no write permission to the file.");
    
    else
      ncdf_handle_error(e, status, "NCDF_CONTROL");

    if((e->KeywordSet(2) || e->KeywordSet(3)) &&e->KeywordPresent(6))
      {
	e->AssureGlobalKW(6);
	delete e->GetKW(6);
	e->GetKW(6)=new DIntGDL(omode);
      }

  }

}
#endif
