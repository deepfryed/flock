#include <ruby/ruby.h>
#include "cluster.h"

#define ID_CONST_GET rb_intern("const_get")
#define CONST_GET(scope, constant) (rb_funcall(scope, ID_CONST_GET, 1, rb_str_new2(constant)))

static VALUE mFlock;

int opt_int_value(VALUE option, char *key, int def) {
  if (NIL_P(option)) return def;

  VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
  return NIL_P(value) ? def : NUM2INT(value);
}

VALUE rb_kmeans(int argc, VALUE *argv, VALUE self) {
    VALUE size, data, mask, weights, options;
    rb_scan_args(argc, argv, "22", &size, &data, &mask, &options);

    if (TYPE(data) != T_ARRAY)
        rb_raise(rb_eArgError, "data should be an array of arrays");

    if (!NIL_P(mask) && TYPE(mask) != T_ARRAY)
        rb_raise(rb_eArgError, "mask should be an array of arrays");

    if (NIL_P(size) || NUM2INT(rb_Integer(size)) > RARRAY_LEN(data))
        rb_raise(rb_eArgError, "size should be > 0 and <= data size");

    int i,j;
    int nrows = RARRAY_LEN(data);
    int ncols = RARRAY_LEN(rb_ary_entry(data, 0));
    int nsets = NUM2INT(rb_Integer(size));

    double **cdata          = (double**)malloc(sizeof(double*)*nrows);
    int    **cmask          = (int   **)malloc(sizeof(int   *)*nrows);
    double **ccentroid      = (double**)malloc(sizeof(double*)*nrows);
    int    **ccentroid_mask = (int   **)malloc(sizeof(int   *)*nrows);
    double *cweights        = (double *)malloc(sizeof(double )*ncols);
    int    *ccluster        = (int    *)malloc(sizeof(int    )*nrows);

    for (i = 0; i < nrows; i++) {
        cdata[i]          = (double*)malloc(sizeof(double)*ncols);
        cmask[i]          = (int   *)malloc(sizeof(int   )*ncols);
        ccentroid[i]      = (double*)malloc(sizeof(double)*ncols);
        ccentroid_mask[i] = (int   *)malloc(sizeof(int   )*ncols);
        for (j = 0; j < ncols; j++) {
            cdata[i][j] = NUM2DBL(rb_Float(rb_ary_entry(rb_ary_entry(data, i), j)));
            cmask[i][j] = NIL_P(mask) ? 1 : NUM2INT(rb_Integer(rb_ary_entry(rb_ary_entry(mask, i), j)));
        }
    }

    weights = NIL_P(options) ? Qnil : rb_hash_aref(options, ID2SYM(rb_intern("weights")));
    for (i = 0; i < ncols; i++) {
        cweights[i] = NIL_P(weights) ? 1.0 : NUM2DBL(rb_Float(rb_ary_entry(weights, i)));
    }

    int transpose = opt_int_value(options, "transpose", 0);
    int npass     = opt_int_value(options, "iterations", 1000);
    // a = average, m = means
    int method    = opt_int_value(options, "method", 'a');
    // e = euclidian,
    // b = city-block distance
    // c = correlation
    // a = absolute value of the correlation
    // u = uncentered correlation
    // x = absolute uncentered correlation
    // s = spearman's rank correlation
    // k = kendall's tau
    int dist      = opt_int_value(options, "metric", 'e');

    int    ifound;
    double error;
    kcluster(nsets,
        nrows, ncols, cdata, cmask, cweights, transpose, npass, method, dist, ccluster, &error, &ifound);

    getclustercentroids(nsets,
        nrows, ncols, cdata, cmask, ccluster, ccentroid, ccentroid_mask, transpose, method);

    VALUE result   = rb_hash_new();
    VALUE cluster  = rb_ary_new();
    VALUE centroid = rb_ary_new();

    for (i = 0; i < nrows; i++) {
        rb_ary_push(cluster, INT2NUM(ccluster[i]));
        VALUE point = rb_ary_new();
        for (j = 0; j < ncols; j++)
            rb_ary_push(point, DBL2NUM(ccentroid[i][j]));
        rb_ary_push(centroid, point);
    }

    rb_hash_aset(result, ID2SYM(rb_intern("cluster")),   cluster);
    rb_hash_aset(result, ID2SYM(rb_intern("centroid")),  centroid);
    rb_hash_aset(result, ID2SYM(rb_intern("error")),     DBL2NUM(error));
    rb_hash_aset(result, ID2SYM(rb_intern("repeated")),  INT2NUM(ifound));

    for (i = 0; i < nrows; i++) {
        free(cdata[i]);
        free(cmask[i]);
        free(ccentroid[i]);
        free(ccentroid_mask[i]);
    }

    free(cdata);
    free(cmask);
    free(ccentroid);
    free(ccentroid_mask);
    free(cweights);
    free(ccluster);

    return result;
}

void Init_flock(void) {
    mFlock = rb_define_module("Flock");
    rb_define_module_function(mFlock, "kmeans", RUBY_METHOD_FUNC(rb_kmeans), -1);

    rb_define_const(mFlock, "METHOD_AVERAGE", INT2NUM('a'));
    rb_define_const(mFlock, "METHOD_MEDIAN",  INT2NUM('m'));

    rb_define_const(mFlock, "METRIC_EUCLIDIAN",                       INT2NUM('e'));
    rb_define_const(mFlock, "METRIC_CITY_BLOCK",                      INT2NUM('b'));
    rb_define_const(mFlock, "METRIC_CORRELATION",                     INT2NUM('c'));
    rb_define_const(mFlock, "METRIC_ABSOLUTE_CORRELATION",            INT2NUM('a'));
    rb_define_const(mFlock, "METRIC_UNCENTERED_CORRELATION",          INT2NUM('u'));
    rb_define_const(mFlock, "METRIC_ABSOLUTE_UNCENTERED_CORRELATION", INT2NUM('x'));
    rb_define_const(mFlock, "METRIC_SPEARMAN",                        INT2NUM('s'));
    rb_define_const(mFlock, "METRIC_KENDALL",                         INT2NUM('k'));
}
