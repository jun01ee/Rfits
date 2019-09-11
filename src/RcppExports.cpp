// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// Rffrtnm
int Rffrtnm(Rcpp::String url, Rcpp::String rootname);
RcppExport SEXP _Rfits_Rffrtnm(SEXP urlSEXP, SEXP rootnameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::String >::type url(urlSEXP);
    Rcpp::traits::input_parameter< Rcpp::String >::type rootname(rootnameSEXP);
    rcpp_result_gen = Rcpp::wrap(Rffrtnm(url, rootname));
    return rcpp_result_gen;
END_RCPP
}
// Rfits_read_img
Rcpp::NumericMatrix Rfits_read_img(Rcpp::String filename, int xpix, int ypix);
RcppExport SEXP _Rfits_Rfits_read_img(SEXP filenameSEXP, SEXP xpixSEXP, SEXP ypixSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::String >::type filename(filenameSEXP);
    Rcpp::traits::input_parameter< int >::type xpix(xpixSEXP);
    Rcpp::traits::input_parameter< int >::type ypix(ypixSEXP);
    rcpp_result_gen = Rcpp::wrap(Rfits_read_img(filename, xpix, ypix));
    return rcpp_result_gen;
END_RCPP
}
// rfits_read_col
Rcpp::NumericVector rfits_read_col(Rcpp::String filename, Rcpp::String column, Rcpp::IntegerVector cwidth, Rcpp::IntegerVector hdu);
RcppExport SEXP _Rfits_rfits_read_col(SEXP filenameSEXP, SEXP columnSEXP, SEXP cwidthSEXP, SEXP hduSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::String >::type filename(filenameSEXP);
    Rcpp::traits::input_parameter< Rcpp::String >::type column(columnSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type cwidth(cwidthSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type hdu(hduSEXP);
    rcpp_result_gen = Rcpp::wrap(rfits_read_col(filename, column, cwidth, hdu));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_Rfits_Rffrtnm", (DL_FUNC) &_Rfits_Rffrtnm, 2},
    {"_Rfits_Rfits_read_img", (DL_FUNC) &_Rfits_Rfits_read_img, 3},
    {"_Rfits_rfits_read_col", (DL_FUNC) &_Rfits_rfits_read_col, 4},
    {NULL, NULL, 0}
};

RcppExport void R_init_Rfits(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
