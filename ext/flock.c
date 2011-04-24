#include <ruby/ruby.h>
#include "cluster.h"

#define ID_CONST_GET rb_intern("const_get")
#define CONST_GET(scope, constant) (rb_funcall(scope, ID_CONST_GET, 1, rb_str_new2(constant)))

static VALUE mFlock;
typedef double (*distance_fn)(int, double**, double**, int**, int**, const double [], int, int, int);

int opt_int_value(VALUE option, char *key, int def) {
  if (NIL_P(option)) return def;

  VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
  return NIL_P(value) ? def : NUM2INT(value);
}

int opt_double_value(VALUE option, char *key, double def) {
  if (NIL_P(option)) return def;

  VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
  return NIL_P(value) ? def : NUM2DBL(value);
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

    int i,j;
    int nrows = RARRAY_LEN(data);
    int ncols = RARRAY_LEN(rb_ary_entry(data, 0));
    int nsets = NUM2INT(rb_Integer(size));

    double **cdata          = (double**)malloc(sizeof(double*)*nrows);
    int    **cmask          = (int   **)malloc(sizeof(int   *)*nrows);
    double *cweights        = (double *)malloc(sizeof(double )*ncols);

    double **ccentroid;
    int *ccluster, **ccentroid_mask, dimx = nrows, dimy = ncols, cdimx = nsets, cdimy = ncols;

    for (i = 0; i < nrows; i++) {
        cdata[i]          = (double*)malloc(sizeof(double)*ncols);
        cmask[i]          = (int   *)malloc(sizeof(int   )*ncols);
        for (j = 0; j < ncols; j++) {
            cdata[i][j] = NUM2DBL(rb_Float(rb_ary_entry(rb_ary_entry(data, i), j)));
            cmask[i][j] = NIL_P(mask) ? 1 : NUM2INT(rb_Integer(rb_ary_entry(rb_ary_entry(mask, i), j)));
        }
    }

    weights = NIL_P(options) ? Qnil : rb_hash_aref(options, ID2SYM(rb_intern("weights")));
    for (i = 0; i < ncols; i++) {
        cweights[i] = NIL_P(weights) ? 1.0 : NUM2DBL(rb_Float(rb_ary_entry(weights, i)));
    }

    if (transpose) {
        dimx  = ncols;
        dimy  = nrows;
        cdimx = nrows;
        cdimy = nsets;
    }

    ccluster       = (int    *)malloc(sizeof(int    )*dimx);
    ccentroid      = (double**)malloc(sizeof(double*)*cdimx);
    ccentroid_mask = (int   **)malloc(sizeof(int   *)*cdimx);

    for (i = 0; i < cdimx; i++) {
      ccentroid[i]      = (double*)malloc(sizeof(double)*cdimy);
      ccentroid_mask[i] = (int   *)malloc(sizeof(int   )*cdimy);
    }

    int    ifound;
    double error;

    kcluster(nsets,
        nrows, ncols, cdata, cmask, cweights, transpose, npass, method, dist, ccluster, &error, &ifound);
    getclustercentroids(nsets,
        nrows, ncols, cdata, cmask, ccluster, ccentroid, ccentroid_mask, transpose, method);

    VALUE result   = rb_hash_new();
    VALUE cluster  = rb_ary_new();
    VALUE centroid = rb_ary_new();

    for (i = 0; i < dimx; i++)
        rb_ary_push(cluster, INT2NUM(ccluster[i]));

    for (i = 0; i < cdimx; i++) {
        VALUE point = rb_ary_new();
        for (j = 0; j < cdimy; j++)
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
    }

    for (i = 0; i < cdimx; i++) {
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

VALUE rb_som(int argc, VALUE *argv, VALUE self) {
    VALUE nx, ny, data, mask, weights, options;
    rb_scan_args(argc, argv, "32", &nx, &ny, &data, &mask, &options);

    if (TYPE(data) != T_ARRAY)
        rb_raise(rb_eArgError, "data should be an array of arrays");

    if (!NIL_P(mask) && TYPE(mask) != T_ARRAY)
        rb_raise(rb_eArgError, "mask should be an array of arrays");

    if (NIL_P(nx) || NUM2INT(rb_Integer(nx)) <= 0)
        rb_raise(rb_eArgError, "nx should be > 0");

    if (NIL_P(ny) || NUM2INT(rb_Integer(ny)) <= 0)
        rb_raise(rb_eArgError, "ny should be > 0");

    int nxgrid    = NUM2INT(rb_Integer(nx));
    int nygrid    = NUM2INT(rb_Integer(ny));
    int transpose = opt_int_value(options, "transpose", 0);
    int npass     = opt_int_value(options, "iterations", 1000);

    // e = euclidian,
    // b = city-block distance
    // c = correlation
    // a = absolute value of the correlation
    // u = uncentered correlation
    // x = absolute uncentered correlation
    // s = spearman's rank correlation
    // k = kendall's tau
    int dist      = opt_int_value(options, "metric", 'e');
    double tau    = opt_double_value(options, "tau", 1.0);

    int i, j, k;
    int nrows = RARRAY_LEN(data);
    int ncols = RARRAY_LEN(rb_ary_entry(data, 0));

    double **cdata          = (double**)malloc(sizeof(double*)*nrows);
    int    **cmask          = (int   **)malloc(sizeof(int   *)*nrows);
    double *cweights        = (double *)malloc(sizeof(double )*ncols);

    int **ccluster;
    double ***ccelldata;
    int dimx = nrows, dimy = ncols;

    if (transpose) {
        dimx = ncols;
        dimy = nrows;
    }

    ccluster = (int **)malloc(sizeof(int*)*dimx);
    for (i = 0; i < dimx; i++)
        ccluster[i] = (int*)malloc(sizeof(int)*2);

    for (i = 0; i < nrows; i++) {
        cdata[i]          = (double*)malloc(sizeof(double)*ncols);
        cmask[i]          = (int   *)malloc(sizeof(int   )*ncols);
        for (j = 0; j < ncols; j++) {
            cdata[i][j] = NUM2DBL(rb_Float(rb_ary_entry(rb_ary_entry(data, i), j)));
            cmask[i][j] = NIL_P(mask) ? 1 : NUM2INT(rb_Integer(rb_ary_entry(rb_ary_entry(mask, i), j)));
        }
    }

    weights = NIL_P(options) ? Qnil : rb_hash_aref(options, ID2SYM(rb_intern("weights")));
    for (i = 0; i < ncols; i++) {
        cweights[i] = NIL_P(weights) ? 1.0 : NUM2DBL(rb_Float(rb_ary_entry(weights, i)));
    }

    ccelldata = (double***)malloc(sizeof(double**)*nxgrid);
    for (i = 0; i < nxgrid; i++) {
        ccelldata[i] = (double **)malloc(sizeof(double*)*nygrid);
        for (j = 0; j < nygrid; j++)
            ccelldata[i][j] = (double *)malloc(sizeof(double)*dimy);
    }

    somcluster(nrows, ncols, cdata, cmask, cweights, transpose,
        nxgrid, nygrid, tau, npass, dist, ccelldata, ccluster);

    VALUE result   = rb_hash_new();
    VALUE cluster  = rb_ary_new();
    VALUE centroid = rb_ary_new();

    for (i = 0; i < dimx; i++)
        rb_ary_push(cluster, INT2NUM(ccluster[i][0] * nxgrid + ccluster[i][1]));

    for (i = 0; i < nxgrid; i++) {
        for (j = 0; j < nygrid; j++) {
            VALUE point = rb_ary_new();
            for (k = 0; k < dimy; k++)
                rb_ary_push(point, DBL2NUM(ccelldata[i][j][k]));
            rb_ary_push(centroid, point);
        }
    }

    rb_hash_aset(result, ID2SYM(rb_intern("cluster")),   cluster);
    rb_hash_aset(result, ID2SYM(rb_intern("centroid")),  centroid);

    for (i = 0; i < nrows; i++) {
        free(cdata[i]);
        free(cmask[i]);
    }

    for (i = 0; i < dimx; i++)
        free(ccluster[i]);

    for (i = 0; i < nxgrid; i++) {
        for (j = 0; j < nygrid; j++)
            free(ccelldata[i][j]);
        free(ccelldata[i]);
    }

    free(cdata);
    free(cmask);
    free(ccelldata);
    free(cweights);
    free(ccluster);

    return result;
}


VALUE rb_distance(VALUE vec1, VALUE vec2, distance_fn fn) {
    uint32_t size;
    double *data1, *data2, *weight, dist;
    int *mask, i;

    if (TYPE(vec1) != T_ARRAY)
        rb_raise(rb_eArgError, "vector1 should be an array");

    if (TYPE(vec2) != T_ARRAY)
        rb_raise(rb_eArgError, "vector2 should be an array");

    size = RARRAY_LEN(vec1);

    if (size != RARRAY_LEN(vec2))
        rb_raise(rb_eArgError, "vector1 & vector2 dimensions mismatch");

    if (size < 1)
        rb_raise(rb_eArgError, "dimension should be greater than 0");

    data1  = (double *)malloc(sizeof(double)*size);
    data2  = (double *)malloc(sizeof(double)*size);
    weight = (double *)malloc(sizeof(double)*size);
    mask   = (int *)malloc(sizeof(int)*size);

    for (i = 0; i < size; i++) {
        mask[i]   = 1;
        weight[i] = 1;
        data1[i]  = NUM2DBL(rb_ary_entry(vec1, i));
        data2[i]  = NUM2DBL(rb_ary_entry(vec2, i));
    }

    dist = fn(size, &data1, &data2, &mask, &mask, weight, 0, 0, 0);
    free(mask);
    free(weight);
    free(data2);
    free(data1);

    return DBL2NUM(dist);
}

VALUE rb_euclid(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, euclid);
}

VALUE rb_cityblock(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, cityblock);
}

VALUE rb_correlation(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, correlation);
}

VALUE rb_ucorrelation(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, ucorrelation);
}

VALUE rb_acorrelation(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, acorrelation);
}

VALUE rb_uacorrelation(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, uacorrelation);
}

VALUE rb_spearman(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, spearman);
}

VALUE rb_kendall(VALUE self, VALUE vec1, VALUE vec2) {
    return rb_distance(vec1, vec2, kendall);
}


void Init_flock(void) {
    mFlock = rb_define_module("Flock");
    rb_define_module_function(mFlock, "kmeans", RUBY_METHOD_FUNC(rb_kmeans), -1);
    rb_define_module_function(mFlock, "self_organizing_map", RUBY_METHOD_FUNC(rb_som), -1);

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

    rb_define_module_function(mFlock, "euclidian_distance", RUBY_METHOD_FUNC(rb_euclid), 2);
    rb_define_module_function(mFlock, "cityblock_distance", RUBY_METHOD_FUNC(rb_cityblock), 2);
    rb_define_module_function(mFlock, "correlation_distance", RUBY_METHOD_FUNC(rb_correlation), 2);
    rb_define_module_function(mFlock, "absolute_correlation_distance", RUBY_METHOD_FUNC(rb_acorrelation), 2);
    rb_define_module_function(mFlock, "uncentered_correlation_distance", RUBY_METHOD_FUNC(rb_ucorrelation), 2);
    rb_define_module_function(mFlock, "absolute_uncentered_correlation_distance", RUBY_METHOD_FUNC(rb_uacorrelation), 2);
    rb_define_module_function(mFlock, "spearman_distance", RUBY_METHOD_FUNC(rb_spearman), 2);
    rb_define_module_function(mFlock, "kendall_distance", RUBY_METHOD_FUNC(rb_kendall), 2);
}
