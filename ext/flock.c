#include <ruby/ruby.h>
#include "cluster.h"

#define ID_CONST_GET rb_intern("const_get")
#define CONST_GET(scope, constant) (rb_funcall(scope, ID_CONST_GET, 1, rb_str_new2(constant)))
#define DEFAULT_ITERATIONS 100

static VALUE mFlock, scFlock;
typedef double (*distance_fn)(int, double**, double**, int**, int**, const double [], int, int, int);

int get_int_option(VALUE option, char *key, int default_value) {
    if (NIL_P(option)) return default_value;

    VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
    return NIL_P(value) ? default_value : NUM2INT(value);
}

int get_bool_option(VALUE option, char *key, int default_value) {
    if (NIL_P(option)) return default_value;
    VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
    return (TYPE(value) == T_FALSE || TYPE(value) == T_NIL) ? 0 : 1;
}

double get_dbl_option(VALUE option, char *key, double default_value) {
    if (NIL_P(option)) return default_value;

    VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
    return NIL_P(value) ? default_value : NUM2DBL(value);
}

VALUE get_value_option(VALUE option, char *key, VALUE default_value) {
    if (NIL_P(option)) return default_value;

    VALUE value = rb_hash_aref(option, ID2SYM(rb_intern(key)));
    return NIL_P(value) ? default_value : value;
}

/* @api private */
VALUE rb_do_kcluster(int argc, VALUE *argv, VALUE self) {
    VALUE size, data, mask, weights, options;
    rb_scan_args(argc, argv, "21", &size, &data, &options);

    if (TYPE(data) != T_ARRAY)
        rb_raise(rb_eArgError, "data should be an array of arrays");

    if (NIL_P(size) || NUM2INT(rb_Integer(size)) > RARRAY_LEN(data))
        rb_raise(rb_eArgError, "size should be > 0 and <= data size");

    mask = get_value_option(options, "mask", Qnil);

    if (!NIL_P(mask) && TYPE(mask) != T_ARRAY)
        rb_raise(rb_eArgError, "mask should be an array of arrays");

    int transpose = get_bool_option(options, "transpose", 0);
    int npass     = get_int_option(options, "iterations", DEFAULT_ITERATIONS);

    // a = average, m = means
    int method    = get_int_option(options, "method", 'a');

    // e = euclidian,
    // b = city-block distance
    // c = correlation
    // a = absolute value of the correlation
    // u = uncentered correlation
    // x = absolute uncentered correlation
    // s = spearman's rank correlation
    // k = kendall's tau
    int dist      = get_int_option(options, "metric", 'e');

    // initial assignment
    int assign    = get_int_option(options, "seed",    0);

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
        nrows, ncols, cdata, cmask, cweights, transpose, npass, method, dist, ccluster, &error, &ifound, assign);
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

/* @api private */
VALUE rb_do_self_organizing_map(int argc, VALUE *argv, VALUE self) {
    VALUE nx, ny, data, mask, weights, options;
    rb_scan_args(argc, argv, "31", &nx, &ny, &data, &options);

    if (TYPE(data) != T_ARRAY)
        rb_raise(rb_eArgError, "data should be an array of arrays");

    mask = get_value_option(options, "mask", Qnil);

    if (!NIL_P(mask) && TYPE(mask) != T_ARRAY)
        rb_raise(rb_eArgError, "mask should be an array of arrays");

    if (NIL_P(nx) || NUM2INT(rb_Integer(nx)) <= 0)
        rb_raise(rb_eArgError, "nx should be > 0");

    if (NIL_P(ny) || NUM2INT(rb_Integer(ny)) <= 0)
        rb_raise(rb_eArgError, "ny should be > 0");

    int nxgrid    = NUM2INT(rb_Integer(nx));
    int nygrid    = NUM2INT(rb_Integer(ny));
    int transpose = get_int_option(options, "transpose", 0);
    int npass     = get_int_option(options, "iterations", DEFAULT_ITERATIONS);

    // e = euclidian,
    // b = city-block distance
    // c = correlation
    // a = absolute value of the correlation
    // u = uncentered correlation
    // x = absolute uncentered correlation
    // s = spearman's rank correlation
    // k = kendall's tau
    int dist      = get_int_option(options, "metric", 'e');
    double tau    = get_dbl_option(options, "tau", 1.0);

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

    somcluster(nrows, ncols, cdata, cmask, cweights, transpose, nxgrid, nygrid, tau, npass, dist, ccelldata, ccluster);

    VALUE result   = rb_hash_new();
    VALUE cluster  = rb_ary_new();
    VALUE centroid = rb_ary_new();

    for (i = 0; i < dimx; i++) {
        VALUE gridpoint = rb_ary_new();
        rb_ary_push(gridpoint, INT2NUM(ccluster[i][0]));
        rb_ary_push(gridpoint, INT2NUM(ccluster[i][1]));
        rb_ary_push(cluster, gridpoint);
    }

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

/* @api private */
VALUE rb_do_treecluster(int argc, VALUE *argv, VALUE self) {
    VALUE size, data, mask, weights, options;
    rb_scan_args(argc, argv, "21", &size, &data, &options);

    if (TYPE(data) != T_ARRAY)
        rb_raise(rb_eArgError, "data should be an array of arrays");

    mask = get_value_option(options, "mask", Qnil);

    if (!NIL_P(mask) && TYPE(mask) != T_ARRAY)
        rb_raise(rb_eArgError, "mask should be an array of arrays");

    if (NIL_P(size) || NUM2INT(rb_Integer(size)) > RARRAY_LEN(data))
        rb_raise(rb_eArgError, "size should be > 0 and <= data size");

    int transpose = get_int_option(options, "transpose", 0);

    // s: pairwise single-linkage clustering
    // m: pairwise maximum- (or complete-) linkage clustering
    // a: pairwise average-linkage clustering
    // c: pairwise centroid-linkage clustering
    int method    = get_int_option(options, "method", 'a');

    // e = euclidian,
    // b = city-block distance
    // c = correlation
    // a = absolute value of the correlation
    // u = uncentered correlation
    // x = absolute uncentered correlation
    // s = spearman's rank correlation
    // k = kendall's tau
    int dist      = get_int_option(options, "metric", 'e');

    int i,j;
    int nrows = RARRAY_LEN(data);
    int ncols = RARRAY_LEN(rb_ary_entry(data, 0));
    int nsets = NUM2INT(rb_Integer(size));

    double **cdata    = (double**)malloc(sizeof(double*)*nrows);
    int    **cmask    = (int   **)malloc(sizeof(int   *)*nrows);
    double *cweights  = (double *)malloc(sizeof(double )*ncols);

    int *ccluster, dimx = nrows, dimy = ncols;

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
    }

    ccluster = (int *)malloc(sizeof(int)*dimx);

    Node *tree   = treecluster(nrows, ncols, cdata, cmask, cweights, transpose, dist, method, 0);
    VALUE result = Qnil, cluster;

    if (tree) {
        cuttree(dimx, tree, nsets, ccluster);

        result  = rb_hash_new();
        cluster = rb_ary_new();

        for (i = 0; i < dimx; i++)
            rb_ary_push(cluster, INT2NUM(ccluster[i]));

        rb_hash_aset(result, ID2SYM(rb_intern("cluster")),   cluster);
    }

    for (i = 0; i < nrows; i++) {
        free(cdata[i]);
        free(cmask[i]);
    }

    free(cdata);
    free(cmask);
    free(cweights);
    free(ccluster);

    if (tree)
        free(tree);
    else
        rb_raise(rb_eNoMemError, "treecluster ran out of memory");

    return result;
}

void inline copy_mask(VALUE src, int *dst, int size, int def) {
    int i;
    if (NIL_P(src))
        for (i = 0; i < size; i++)
            dst[i] = def;
    else
        for (i = 0; i < size; i++)
            dst[i] = NUM2INT(rb_ary_entry(src, i));
}

VALUE rb_distance(VALUE vec1, VALUE m1, VALUE vec2, VALUE m2, distance_fn fn) {
    uint32_t size;
    double *data1, *data2, *weight, dist;
    int *mask1, *mask2, i;

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
    mask1  = (int *)malloc(sizeof(int)*size);
    mask2  = (int *)malloc(sizeof(int)*size);

    for (i = 0; i < size; i++) {
        weight[i] = 1;
        data1[i]  = NUM2DBL(rb_ary_entry(vec1, i));
        data2[i]  = NUM2DBL(rb_ary_entry(vec2, i));
    }

    copy_mask(m1, mask1, size, 1);
    copy_mask(m2, mask2, size, 1);

    dist = fn(size, &data1, &data2, &mask1, &mask2, weight, 0, 0, 0);
    free(mask1);
    free(mask2);
    free(weight);
    free(data2);
    free(data1);

    return DBL2NUM(dist);
}

/*
  Euclidian distance measure

  @example
    Flock.euclidian_distance([0, 0], [1, 1])
    Flock.euclidian_distance([0, 0, 0], [1, 1, 1], [1, 1, 0], [1, 1, 0]) # with mask

  @overload euclidian_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_euclid(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, euclid);
}

/*
  Cityblock distance measure

  @overload cityblock_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_cityblock(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, cityblock);
}

/*
  Correlation distance measure

  @overload correlation_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_correlation(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, correlation);
}

/*
  Uncentered correlation distance measure

  @overload uncentered_correlation_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_ucorrelation(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, ucorrelation);
}

/*
  Absolute correlation distance measure

  @overload absolute_correlation_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_acorrelation(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, acorrelation);
}

/*
  Absolute uncentered correlation distance measure

  @overload absolute_uncentered_correlation_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_uacorrelation(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, uacorrelation);
}

/*
  Spearman distance measure

  @overload spearman_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_spearman(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, spearman);
}

/*
  Kendall distance measure

  @overload kendall_distance(vector1, vector2, mask1 = identity, mask2 = identity)
    @param [Array]  vector1     Numeric vector
    @param [Array]  vector2     Numeric vector
    @param [Array]  mask1       Optional mask for vector1
    @param [Array]  mask2       Optional mask for vector2
*/
VALUE rb_kendall(int argc, VALUE *argv, VALUE self) {
    VALUE v1, v2, m1, m2;
    rb_scan_args(argc, argv, "22", &v1, &v2, &m1, &m2);
    return rb_distance(v1, m1, v2, m2, kendall);
}


void Init_flock(void) {
    mFlock  = rb_define_module("Flock");
    scFlock = rb_singleton_class(mFlock);

    rb_define_private_method(scFlock, "do_kcluster",            RUBY_METHOD_FUNC(rb_do_kcluster),            -1);
    rb_define_private_method(scFlock, "do_self_organizing_map", RUBY_METHOD_FUNC(rb_do_self_organizing_map), -1);
    rb_define_private_method(scFlock, "do_treecluster",         RUBY_METHOD_FUNC(rb_do_treecluster),         -1);

    /* kcluster method - K-Means */
    rb_define_const(mFlock, "METHOD_AVERAGE", INT2NUM('a'));

    /* kcluster method - K-Medians */
    rb_define_const(mFlock, "METHOD_MEDIAN",  INT2NUM('m'));

    /* treecluster method - pairwise single-linkage clustering */
    rb_define_const(mFlock, "METHOD_SINGLE_LINKAGE",   INT2NUM('s'));
    /* treecluster method - pairwise maximum- (or complete-) linkage clustering */
    rb_define_const(mFlock, "METHOD_MAXIMUM_LINKAGE",  INT2NUM('m'));
    /* treecluster method - pairwise average-linkage clustering */
    rb_define_const(mFlock, "METHOD_AVERAGE_LINKAGE",  INT2NUM('a'));
    /* treecluster method - pairwise centroid-linkage clustering */
    rb_define_const(mFlock, "METHOD_CENTROID_LINKAGE", INT2NUM('c'));


    rb_define_const(mFlock, "METRIC_EUCLIDIAN",                       INT2NUM('e'));
    rb_define_const(mFlock, "METRIC_CITY_BLOCK",                      INT2NUM('b'));
    rb_define_const(mFlock, "METRIC_CORRELATION",                     INT2NUM('c'));
    rb_define_const(mFlock, "METRIC_ABSOLUTE_CORRELATION",            INT2NUM('a'));
    rb_define_const(mFlock, "METRIC_UNCENTERED_CORRELATION",          INT2NUM('u'));
    rb_define_const(mFlock, "METRIC_ABSOLUTE_UNCENTERED_CORRELATION", INT2NUM('x'));
    rb_define_const(mFlock, "METRIC_SPEARMAN",                        INT2NUM('s'));
    rb_define_const(mFlock, "METRIC_KENDALL",                         INT2NUM('k'));

    /* Randomly assign data points to clusters using a uniform distribution. */
    rb_define_const(mFlock, "SEED_RANDOM",          INT2NUM(0));

    /*
        K-Means++ style initialization where data points are probabilistically assigned to clusters
        based on their distance from closest cluster.
    */
    rb_define_const(mFlock, "SEED_KMEANS_PLUSPLUS", INT2NUM(1));

    /*
        Deterministic cluster assignment by spreading out initial clusters as far away from each other
        as possible.
    */
    rb_define_const(mFlock, "SEED_SPREADOUT",       INT2NUM(2));

    rb_define_module_function(mFlock, "euclidian_distance", RUBY_METHOD_FUNC(rb_euclid), -1);
    rb_define_module_function(mFlock, "cityblock_distance", RUBY_METHOD_FUNC(rb_cityblock), -1);
    rb_define_module_function(mFlock, "correlation_distance", RUBY_METHOD_FUNC(rb_correlation), -1);
    rb_define_module_function(mFlock, "absolute_correlation_distance", RUBY_METHOD_FUNC(rb_acorrelation), -1);
    rb_define_module_function(mFlock, "uncentered_correlation_distance", RUBY_METHOD_FUNC(rb_ucorrelation), -1);
    rb_define_module_function(mFlock, "absolute_uncentered_correlation_distance", RUBY_METHOD_FUNC(rb_uacorrelation), -1);
    rb_define_module_function(mFlock, "spearman_distance", RUBY_METHOD_FUNC(rb_spearman), -1);
    rb_define_module_function(mFlock, "kendall_distance", RUBY_METHOD_FUNC(rb_kendall), -1);
}
