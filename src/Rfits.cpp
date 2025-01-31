#include <algorithm>
#include <utility>
#include <vector>
#include <Rcpp.h>

#include "cfitsio/fitsio.h"

// Comments with Rcout << something here << std::endl;

using namespace Rcpp;

std::runtime_error fits_status_to_exception(const char *func_name, int status)
{
    char err_msg[30];
    fits_get_errstatus(status, err_msg);
    std::ostringstream os;
    os << "Error when invoking fits_" << func_name << ": " << err_msg;
    throw std::runtime_error(os.str());
}


/**
 * Utility class that takes ownership of a fitsfile pointer
 * and closes it automatically at destruction time.
 */
class fits_file {
public:
  fits_file() {}
  fits_file(fitsfile *fptr) : m_fptr(fptr) {}
  fits_file(const fits_file &other) = default;
  fits_file(fits_file &&other) = default;
  ~fits_file()
  {
    if (m_fptr) {
      int status = 0;
      fits_close_file(m_fptr, &status);
    }
  }

  operator fitsfile*()
  {
    return m_fptr;
  }

  operator fitsfile**()
  {
    return &m_fptr;
  }

  fits_file &operator=(fitsfile *fptr)
  {
    m_fptr = fptr;
	 return *this;
  }

  fitsfile *m_fptr = nullptr;
};

/**
 * Utility function to convert FITS API call errors into exceptions.
 *
 * Notably, this doesn't work with fits_open_file as this name is actually
 * defined as a macro function, so it cannot be just passed around.
 */
template <typename F, typename ... Args>
void _fits_invoke(const char *func_name, F&& func, Args&& ... args)
{
  int status = 0;
  func(std::forward<Args>(args)..., &status);
  if (status) {
    throw fits_status_to_exception(func_name, status);
  }
}

fitsfile *fits_safe_open_file(const char *filename, int mode)
{
  int status = 0;
  fitsfile *file;
  fits_open_file(&file, const_cast<char *>(filename), mode, &status);
  if (status) {
    throw fits_status_to_exception("open_file", status);
  }
  return file;
}

#define fits_invoke(F, ...) _fits_invoke(#F, fits_ ## F, __VA_ARGS__)

std::vector<char *> to_string_vector(const Rcpp::CharacterVector &strings)
{
  std::vector<char *> c_strings(strings.size());
  std::transform(strings.begin(), strings.end(), c_strings.begin(),
                 [](const Rcpp::String &string) {
                   return const_cast<char *>(string.get_cstring());
                 }
  );
  return c_strings;
}

// [[Rcpp::export]]
void Cfits_create_header(Rcpp::String filename, int create_ext=1, int create_file=1)
{
  // make empty FITS, useful for just the first header componenet of multi-extension file.
  int nhdu,hdutype;
  fits_file fptr;
  int naxis=0;
  long *axes = {0};
  
  if(create_file == 1){
    fits_invoke(create_file, fptr, filename.get_cstring());
    fits_invoke(create_hdu, fptr);
    fits_invoke(create_img, fptr, 16, naxis, axes);
  }else{
    if(create_ext == 1){
      fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
      fits_invoke(get_num_hdus, fptr, &nhdu);
      fits_invoke(movabs_hdu, fptr, nhdu, &hdutype);
      fits_invoke(create_hdu, fptr);
    }
  }
}
  
// [[Rcpp::export]]
SEXP Cfits_read_col(Rcpp::String filename, int colref=1, int ext=2, long nrow=0){

  int hdutype,anynull,typecode,ii;
  long repeat,width;

  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_coltype, fptr, colref, &typecode, &repeat, &width);
  if(nrow==0){
    fits_invoke(get_num_rows, fptr, &nrow);
  }
  
  if ( typecode == TSTRING ) {
    int cwidth;
    fits_invoke(get_col_display_width, fptr, colref, &cwidth);

    char **data = (char **)malloc(sizeof(char *) * nrow);
    for (ii = 0 ; ii < nrow ; ii++ ) {
      data[ii] = (char*)calloc(cwidth + 1, 1);
    }
    fits_invoke(read_col, fptr, TSTRING, colref, 1, 1, nrow, nullptr, data, &anynull);
    Rcpp::StringVector out(nrow);
    std::copy(data, data + nrow, out.begin());
    for (int i = 0; i != nrow; i++) {
      free(data[i]);
    }
    free(data);
    return out;
  }
  else if ( typecode == TBIT ) {
    int nullval = 0;
    std::vector<Rbyte> col(nrow);
    fits_invoke(read_col, fptr, TBIT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::LogicalVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TLOGICAL ) {
    int nullval = 0;
    std::vector<Rbyte> col(nrow);
    fits_invoke(read_col, fptr, TLOGICAL, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::LogicalVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TBYTE ) {
    int nullval = 0;
    // std::vector<Rbyte> col(nrow);
    std::vector<Rbyte> col(nrow);
    fits_invoke(read_col, fptr, TBYTE, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TINT ) {
    int nullval = -999;
    std::vector<int> col(nrow);
    fits_invoke(read_col, fptr, TINT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TUINT ) {
    unsigned int nullval = 0;
    std::vector<unsigned int> col(nrow);
    fits_invoke(read_col, fptr, TUINT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TINT32BIT ) {
    long nullval = 0;
    std::vector<long> col(nrow);
    fits_invoke(read_col, fptr, TINT32BIT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TSHORT ) {
    short nullval = -128;
    std::vector<short> col(nrow);
    fits_invoke(read_col, fptr, TSHORT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TUSHORT ) {
    unsigned short nullval = 255;
    std::vector<unsigned short> col(nrow);
    fits_invoke(read_col, fptr, TUSHORT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::IntegerVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TFLOAT ) {
    float nullval = -999;
    std::vector<float> col(nrow);
    fits_invoke(read_col, fptr, TFLOAT, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::NumericVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TLONG ) {
    long nullval = -999;
    std::vector<long> col(nrow);
    fits_invoke(read_col, fptr, TLONG, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::NumericVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  else if ( typecode == TLONGLONG ) {
    long nullval = -999;
    std::vector<int64_t> col(nrow);
    fits_invoke(read_col, fptr, TLONGLONG, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::NumericVector out(nrow);
    std::memcpy(&(out[0]), &(col[0]), nrow * sizeof(double));
    out.attr("class") = "integer64";
    return out;
  }
  else if ( typecode == TDOUBLE ) {
    double nullval = -999;
    std::vector<double> col(nrow);
    fits_invoke(read_col, fptr, TDOUBLE, colref, 1, 1, nrow, &nullval, col.data(), &anynull);
    Rcpp::NumericVector out(nrow);
    std::copy(col.begin(), col.end(), out.begin());
    return out;
  }
  throw std::runtime_error("unsupported type");
}

// [[Rcpp::export]]
int Cfits_read_nrow(Rcpp::String filename, int ext=2){
  int hdutype;
  long nrow;

  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_num_rows, fptr, &nrow);
  return nrow;
}

// [[Rcpp::export]]
int Cfits_read_nhdu(Rcpp::String filename){
  int nhdu;

  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(get_num_hdus, fptr, &nhdu);
  return nhdu;
}

// [[Rcpp::export]]
int Cfits_read_ncol(Rcpp::String filename, int ext=2){
  int hdutype,ncol;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_num_cols, fptr,&ncol);
  return ncol;
}

// [[Rcpp::export]]
SEXP Cfits_read_colname(Rcpp::String filename, int colref=1, int ext=2){
  int hdutype, ncol;

  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_num_cols, fptr, &ncol);

  Rcpp::StringVector out(ncol);

  char colname[81];

  int status = 0;
  int ii = 0;
  while ( status != COL_NOT_FOUND & ii < ncol) {
    fits_get_colname(fptr, CASEINSEN, (char *)"*", (char *)colname, &colref, &status);
    if (status != COL_NOT_FOUND) {
      out[ii] = colname;
    }
    ii++;
  }
  return out;
}

// [[Rcpp::export]]
void Cfits_create_bintable(Rcpp::String filename, int tfields,
                         Rcpp::CharacterVector ttypes, Rcpp::CharacterVector tforms,
                         Rcpp::CharacterVector tunits, Rcpp::String extname, int ext=2,
                         int create_ext=1, int create_file=1, int table_type=2)
{
  auto c_ttypes = to_string_vector(ttypes);
  auto c_tforms = to_string_vector(tforms);
  auto c_tunits = to_string_vector(tunits);

  int nhdu, hdutype;
  
  fits_file fptr;
  
  if(create_file == 1){
    fits_invoke(create_file, fptr, filename.get_cstring());
    fits_invoke(create_hdu, fptr);
  }else{
    fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
    if(create_ext == 1){
      fits_invoke(get_num_hdus, fptr, &nhdu);
      fits_invoke(movabs_hdu, fptr, nhdu, &hdutype);
      fits_invoke(create_hdu, fptr);
    }else{
      fits_invoke(movabs_hdu, fptr, ext, &hdutype);
      fits_invoke(delete_hdu, fptr, &hdutype);
    }
  }
  
  fits_invoke(create_tbl, fptr, table_type, 0, tfields,
              c_ttypes.data(), c_tforms.data(), c_tunits.data(),
              (char *)extname.get_cstring());
}

// [[Rcpp::export]]
void Cfits_write_col(Rcpp::String filename, SEXP data, int nrow, int colref=1, int ext=2, int typecode=1){
  
  int hdutype,ii;
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);

  if ( typecode == TSTRING ) {
    std::vector<char *> s_data(nrow);
    for (ii = 0 ; ii < nrow ; ii++ ) {
      s_data[ii] = (char*)CHAR(STRING_ELT(data, ii));
    }
    fits_invoke(write_col, fptr, typecode, colref, 1, 1, nrow, s_data.data());
  }else if (typecode == TBIT){
    fits_invoke(write_col, fptr, typecode, colref, 1, 1, nrow, INTEGER(data));
  }else if (typecode == TINT){
    fits_invoke(write_col, fptr, typecode, colref, 1, 1, nrow, INTEGER(data));
  }else if(typecode == TLONGLONG){
    fits_invoke(write_col, fptr, typecode, colref, 1, 1, nrow, REAL(data));
  }else if(typecode == TDOUBLE){
    fits_invoke(write_col, fptr, typecode, colref, 1, 1, nrow, REAL(data));
  }
}

// int CFITS_API ffgkey(fitsfile *fptr, const char *keyname, char *keyval, char *comm,
//                      int *status);
// 
// int CFITS_API ffgky( fitsfile *fptr, int datatype, const char *keyname, void *value,
//                      char *comm, int *status);
// [[Rcpp::export]]
SEXP Cfits_read_key(Rcpp::String filename, Rcpp::String keyname, int typecode, int ext=1){
  int hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  char comment[81];
  
  if ( typecode == TDOUBLE ) {
    Rcpp::NumericVector out(1);
    std::vector<double> keyvalue(1);
    fits_invoke(read_key, fptr, TDOUBLE, keyname.get_cstring(), keyvalue.data(), comment);
    std::copy(keyvalue.begin(), keyvalue.end(), out.begin());
    return(out);
  }else if ( typecode == TSTRING){
    Rcpp::StringVector out(1);
    //std::vector<std::string> keyvalue(1);
    char keyvalue[81];
    fits_invoke(read_key, fptr, TSTRING, keyname.get_cstring(), keyvalue, comment);
    out[0] = keyvalue;
    //std::copy(keyvalue.begin(), keyvalue.end(), out.begin());
    return(out);
  }else if ( typecode == TLONG ) {
    Rcpp::IntegerVector out(1);
    std::vector<int> keyvalue(1);
    fits_invoke(read_key, fptr, TLONG, keyname.get_cstring(), keyvalue.data(), comment);
    std::copy(keyvalue.begin(), keyvalue.end(), out.begin());
    return(out);
  }
  throw std::runtime_error("unsupported type");
}

// [[Rcpp::export]]
void Cfits_update_key(Rcpp::String filename, SEXP keyvalue, Rcpp::String keyname,
                      Rcpp::String keycomment, int ext=1, int typecode=1){
  int hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  if(typecode==TSTRING){
    char *s_keyvalue;
    s_keyvalue = (char*)CHAR(STRING_ELT(keyvalue, 0));
    fits_invoke(update_key, fptr, typecode, keyname.get_cstring(), s_keyvalue, keycomment.get_cstring());
  }else if (typecode == TINT){
    fits_invoke(update_key, fptr, typecode, keyname.get_cstring(), INTEGER(keyvalue), keycomment.get_cstring());
  }else if(typecode == TLONGLONG){
    fits_invoke(update_key, fptr, typecode, keyname.get_cstring(), REAL(keyvalue), keycomment.get_cstring());
  }else if(typecode == TDOUBLE){
    fits_invoke(update_key, fptr, typecode, keyname.get_cstring(), REAL(keyvalue), keycomment.get_cstring());
  }else if(typecode == TLOGICAL){
    fits_invoke(update_key, fptr, typecode, keyname.get_cstring(), INTEGER(keyvalue), keycomment.get_cstring());
  }
}

// [[Rcpp::export]]
void Cfits_write_history(Rcpp::String filename, Rcpp::String history, int ext=1){
  int hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  fits_invoke(write_history, fptr, history.get_cstring());
}

// [[Rcpp::export]]
void Cfits_write_comment(Rcpp::String filename, Rcpp::String comment, int ext=1){
  int hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  fits_invoke(write_comment, fptr, comment.get_cstring());
}

// [[Rcpp::export]]
void Cfits_write_date(Rcpp::String filename, int ext=1){
  int hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  fits_invoke(write_date, fptr);
}


//fitsfile *fptr, int bitpix, int naxis, long *naxes, int *status
// BYTE_IMG      =   8   ( 8-bit byte pixels, 0 - 255)
//   SHORT_IMG     =  16   (16 bit integer pixels)
//   LONG_IMG      =  32   (32-bit integer pixels)
//   LONGLONG_IMG  =  64   (64-bit integer pixels)
//   FLOAT_IMG     = -32   (32-bit floating point pixels)
//   DOUBLE_IMG    = -64   (64-bit floating point pixels)
//fits_write_pix(fitsfile *fptr, int datatype, long *fpixel,
//               long nelements, void *array, int *status);
// [[Rcpp::export]]
void Cfits_create_image(Rcpp::String filename, int naxis, long naxis1=100 , long naxis2=100, long naxis3=1,
                        long naxis4=1, int ext=1, int create_ext=1, int create_file=1, int bitpix=32)
{
  int hdutype;
  fits_file fptr;
  
  long naxes_vector[] = {naxis1};
  long naxes_image[] = {naxis1, naxis2};
  long naxes_cube[] = {naxis1, naxis2, naxis3};
  long naxes_array[] = {naxis1, naxis2, naxis3, naxis4};
  long *axes = (naxis == 1) ? naxes_vector : (naxis == 2) ? naxes_image : (naxis == 3 ? naxes_cube : naxes_array);
  
  if(create_file == 1){
    fits_invoke(create_file, fptr, filename.get_cstring());
    fits_invoke(create_hdu, fptr);
  }else{
    fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
    if(create_ext == 1){
      int nhdu;
      fits_invoke(get_num_hdus, fptr, &nhdu);
      fits_invoke(movabs_hdu, fptr, nhdu, &hdutype);
      fits_invoke(create_hdu, fptr);
    }else{
      fits_invoke(movabs_hdu, fptr, ext, &hdutype);
      fits_invoke(delete_hdu, fptr, &hdutype);
    }
  }
  
  fits_invoke(create_img, fptr, bitpix, naxis, axes);
}

// [[Rcpp::export]]
void Cfits_write_pix(Rcpp::String filename, SEXP data, int datatype,
                     int naxis, long naxis1=100 , long naxis2=100, long naxis3=1,
                     long naxis4=1, int ext=1)
{
  int hdutype, ii;
  fits_file fptr;
  long nelements = naxis1 * naxis2 * naxis3 * naxis4;
  
  long fpixel_vector[] = {1};
  long fpixel_image[] = {1, 1};
  long fpixel_cube[] = {1, 1, 1};
  long fpixel_array[] = {1, 1, 1, 1};
  long *fpixel = (naxis == 1) ? fpixel_vector : (naxis == 2) ? fpixel_image : (naxis == 3 ? fpixel_cube : fpixel_array);
  
  fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  //below need to work for integers and doubles:
  if(datatype == TBYTE){
    // char *data_b = (char *)malloc(nelements * sizeof(char));
    std::vector<Rbyte> data_b(nelements);
    for (ii = 0; ii < nelements; ii++)  {
      data_b[ii] = INTEGER(data)[ii];
    }
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, data_b.data());
  }else if(datatype == TINT){
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, INTEGER(data));
  }else if(datatype == TSHORT){
    // short *data_s = (short *)malloc(nelements * sizeof(short));
    std::vector<short> data_s(nelements);
    for (ii = 0; ii < nelements; ii++)  {
      data_s[ii] = INTEGER(data)[ii];
    } 
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, data_s.data());
  }else if(datatype == TLONG){
    // long *data_l = (long *)malloc(nelements * sizeof(long));
    std::vector<long> data_l(nelements);
    for (ii = 0; ii < nelements; ii++)  {
      data_l[ii] = INTEGER(data)[ii];
    }
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, data_l.data());
  }else if(datatype == TLONGLONG){
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, REAL(data));
  }else if(datatype == TDOUBLE){
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, REAL(data));
  }else if(datatype == TFLOAT){
    // float *data_f = (float *)malloc(nelements * sizeof(float));
    std::vector<float> data_f(nelements);
    for (ii = 0; ii < nelements; ii++)  {
      data_f[ii] = REAL(data)[ii];
    }
    fits_invoke(write_pix, fptr, datatype, fpixel, nelements, data_f.data());
  }
}

// [[Rcpp::export]]
SEXP Cfits_read_img(Rcpp::String filename, long naxis1=100, long naxis2=100, long naxis3=1,
                    long naxis4=1, int ext=1, int datatype=-32)
{
  int anynull, nullvals = 0, hdutype;

  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);

  long npixels = naxis1 * naxis2 * naxis3 * naxis4;

  if (datatype==FLOAT_IMG){
    std::vector<float> pixels(npixels);
    fits_invoke(read_img, fptr, TFLOAT, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==DOUBLE_IMG){
    std::vector<double> pixels(npixels);
    fits_invoke(read_img, fptr, TDOUBLE, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==BYTE_IMG){
    //std::vector<char> pixels(npixels);
    std::vector<Rbyte> pixels(npixels);
    fits_invoke(read_img, fptr, TBYTE, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==SHORT_IMG){
// Weirdly we need to use longs here to deal with the scenario of BZERO making the unsigned short too large
    std::vector<long> pixels(npixels);
    fits_invoke(read_img, fptr, TLONG, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==LONG_IMG){
    std::vector<long> pixels(npixels);
    fits_invoke(read_img, fptr, TLONG, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==LONGLONG_IMG){
    std::vector<int64_t> pixels(npixels);
    fits_invoke(read_img, fptr, TLONGLONG, 1, npixels, &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::memcpy(&(pixel_matrix[0]), &(pixels[0]), npixels * sizeof(double));
    pixel_matrix.attr("class") = "integer64";
    return(pixel_matrix);
  }
  throw std::runtime_error("unsupported type");
}

// [[Rcpp::export]]
SEXP Cfits_read_header(Rcpp::String filename, int ext=1){
  int nkeys, keypos, ii, hdutype;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_hdrpos, fptr, &nkeys, &keypos);
  
  Rcpp::StringVector out(nkeys);
  char card[FLEN_CARD];
  
  for (ii = 1; ii <= nkeys; ii++)  {
    fits_invoke(read_record, fptr, ii, card);
    out[ii-1] = card;
  }
  return(out);
}

// [[Rcpp::export]]
SEXP Cfits_read_header_raw(Rcpp::String filename, int ext=1){
  int nkeys, keypos, hdutype;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_hdrpos, fptr, &nkeys, &keypos);
  
  Rcpp::StringVector out(1);
  
  char *header = (char *)malloc(FLEN_CARD * nkeys);
  
  fits_invoke(hdr2str, fptr, 1, nullptr, 0, &header, &nkeys);
  
  out[0] = header;
  
  free(header);
  
  return(out);
}

// [[Rcpp::export]]
void Cfits_delete_HDU(Rcpp::String filename, int ext=1){
  int hdutype;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(delete_hdu, fptr, &hdutype);
}

// [[Rcpp::export]]
void Cfits_delete_key(Rcpp::String filename, Rcpp::String keyname, int ext=1){
  int hdutype;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(delete_key, fptr, keyname.get_cstring());
}

// [[Rcpp::export]]
void Cfits_delete_header(Rcpp::String filename, int ext=1){
  int hdutype, nkeys, keypos, ii;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READWRITE);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_hdrpos, fptr, &nkeys, &keypos);
  for (ii = 2; ii <= nkeys; ii++)  {
    fits_invoke(delete_record, fptr, 2);
  }
}

// [[Rcpp::export]]
SEXP Cfits_read_img_subset(Rcpp::String filename, long fpixel0=1, long fpixel1=1, long fpixel2=1, long fpixel3=1,
                           long lpixel0=100, long lpixel1=100, long lpixel2=1, long lpixel3=1, int ext=1, int datatype=-32)
{
  int anynull, nullvals = 0, hdutype;
  
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  
  long fpixel[] = {fpixel0, fpixel1, fpixel2, fpixel3};
  long lpixel[] = {lpixel0, lpixel1, lpixel2, lpixel3};
  
  int naxis1 = (lpixel[0] - fpixel[0] + 1);
  int naxis2 = (lpixel[1] - fpixel[1] + 1);
  int naxis3 = (lpixel[2] - fpixel[2] + 1);
  int naxis4 = (lpixel[3] - fpixel[3] + 1);
  int npixels = naxis1 * naxis2 * naxis3 * naxis4;
  long inc[] = {1, 1, 1, 1};
  
  if (datatype==FLOAT_IMG){
    std::vector<float> pixels(npixels);
    fits_invoke(read_subset, fptr, TFLOAT, fpixel, lpixel, inc,
                  &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==DOUBLE_IMG){
    std::vector<double> pixels(npixels);
    fits_invoke(read_subset, fptr, TDOUBLE, fpixel, lpixel, inc,
                  &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==BYTE_IMG){
    std::vector<char> pixels(npixels);
    fits_invoke(read_subset, fptr, TBYTE, fpixel, lpixel, inc,
                  &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==SHORT_IMG){
    std::vector<short> pixels(npixels);
    fits_invoke(read_subset, fptr, TSHORT, fpixel, lpixel, inc,
                  &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==LONG_IMG){
    std::vector<long> pixels(npixels);
    fits_invoke(read_subset, fptr, TLONG, fpixel, lpixel, inc,
                  &nullvals, pixels.data(), &anynull);
    Rcpp::IntegerVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::copy(pixels.begin(), pixels.end(), pixel_matrix.begin());
    return(pixel_matrix);
  }else if (datatype==LONGLONG_IMG){
    std::vector<int64_t> pixels(npixels);
    fits_invoke(read_subset, fptr, TLONG, fpixel, lpixel, inc,
                &nullvals, pixels.data(), &anynull);
    Rcpp::NumericVector pixel_matrix(naxis1 * naxis2 * naxis3 * naxis4);
    std::memcpy(&(pixel_matrix[0]), &(pixels[0]), npixels * sizeof(double));
    pixel_matrix.attr("class") = "integer64";
    return(pixel_matrix);
  }
  throw std::runtime_error("unsupported type");
}

// [[Rcpp::export]]
void Cfits_write_chksum(Rcpp::String filename){
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(write_chksum, fptr);
}

// [[Rcpp::export]]
SEXP Cfits_verify_chksum(Rcpp::String filename, int verbose){
  int dataok, hduok;
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(verify_chksum, fptr, &dataok, &hduok);
  if(verbose == 1){
    if(dataok == 1){
      Rcpp::Rcout << "DATASUM is correct\n";
    }
    if(dataok == 0){
      Rcpp::Rcout << "DATASUM is missing\n";
    }
    if(dataok == -1){
      Rcpp::Rcout << "DATASUM is incorrect\n";
    }
    if(hduok == 1){
      Rcpp::Rcout << "CHECKSUM is correct\n";
    }
    if(hduok == 0){
      Rcpp::Rcout << "CHECKSUM is missing\n";
    }
    if(hduok == -1){
      Rcpp::Rcout << "CHECKSUM is incorrect\n";
    }
  }
  Rcpp::IntegerVector out(2);
  out[0] = dataok;
  out[1] = hduok;
  return(out);
}

// [[Rcpp::export]]
SEXP Cfits_get_chksum(Rcpp::String filename){
  unsigned long datasum, hdusum;
  fits_file fptr = fits_safe_open_file(filename.get_cstring(), READWRITE);
  fits_invoke(get_chksum, fptr, &datasum, &hdusum);
  Rcpp::NumericVector out(2);
  out.attr("class") = "integer64";
  std::memcpy(&(out[0]), &datasum, 8);
  std::memcpy(&(out[1]), &hdusum, 8);
  return(out);
}

// [[Rcpp::export]]
SEXP Cfits_encode_chksum(unsigned long sum, int complement=0){
  char ascii[16];
  fits_encode_chksum(sum, complement, (char *)ascii);
  Rcpp::StringVector out;
  out = ascii;
  return(out);
}

// [[Rcpp::export]]
SEXP Cfits_decode_chksum(Rcpp::String ascii, int complement=0){
  unsigned long sum;
  fits_decode_chksum((char *)ascii.get_cstring(), complement, &sum);
  Rcpp::NumericVector out(1);
  out.attr("class") = "integer64";
  std::memcpy(&(out[0]), &sum, 8);
  return(out);
}

// [[Rcpp::export]]
int Cfits_read_nkey(Rcpp::String filename, int ext=1){
  int nkeys, keypos, hdutype;
  fits_file fptr;
  fits_invoke(open_image, fptr, filename.get_cstring(), READONLY);
  fits_invoke(movabs_hdu, fptr, ext, &hdutype);
  fits_invoke(get_hdrpos, fptr, &nkeys, &keypos);
  return(nkeys);
}

