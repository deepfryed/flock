#include <stdlib.h>

extern double uniform();
typedef struct clusterpoint {
  double dist;
  int n, chosen, closest;
} clusterpoint;

int compare(const void *ptr1, const void *ptr2) {
  clusterpoint *p1 = (clusterpoint *)ptr1, *p2 = (clusterpoint *)ptr2;
  return p1->dist == p2->dist ? 0 : p1->dist < p2->dist ? -1 : 1;
}

double compute_distances(int ndata, int npoints,
  double **data, int **mask, double weight[], int transpose, clusterpoint dists[],
  double (*metric)(int, double**, double**, int**, int**, const double[], int, int, int)) {

  int i, j, closest = 0;
  double min, dist, total = 0;

  // compute distances to chosen point
  for (i = 0; i < npoints; i++) {
    if (dists[i].chosen) continue;

    min = -1;
    for (j = 0; j < npoints; j++) {
      if (!dists[j].chosen) continue;

      dist = metric(ndata, data, data, mask, mask, weight, dists[i].n, dists[j].n, transpose);
      //printf("i: %d j: %d d: %.2f\n", dists[i].n, dists[j].n, dist);
      if (min < 0 || min > dist) {
        min     = dist;
        closest = j;
      }
    }

    dists[i].dist    = min * min;
    dists[i].closest = closest;
    total           += dists[i].dist;
  }

  return total;
}

void weightedassign(int nclusters, int nrows, int ncolumns,
  double** data, int** mask, double weight[], int transpose,
  double (*metric)(int, double**, double**, int**, int**, const double[], int, int, int),
  int clusterid[]) {

  int i, n, chosen = (int)((double)nrows*uniform());
  int ndata = (transpose == 0 ? ncolumns : nrows), npoints = (transpose == 0 ? nrows : ncolumns);
  double total = 0, cutoff, curr;
  clusterpoint dists[npoints];

  for (i = 0; i < npoints; i++) {
    dists[i].n      = i;
    dists[i].chosen = 0;
    dists[i].dist   = 0;
  }

  // setup 1st centroid
  n                    = 1;
  clusterid[chosen]    = 0;
  dists[chosen].chosen = 1;

  // pick k-points for k-clusters with a probability weighted by square of distance from closest centroid.
  while (n < nclusters) {
    total = compute_distances(ndata, npoints, data, mask, weight, transpose, dists, metric);
    qsort((void*)dists, npoints, sizeof(clusterpoint), compare);

    curr   = 0;
    cutoff = total * uniform();
    for (i = 0; i < npoints; i++) {
      if (dists[i].chosen) continue;
      curr += dists[i].dist;
      if (curr >= cutoff || i == (npoints - 1)) {
        clusterid[dists[i].n] = n++;
        dists[i].chosen       = 1;
        dists[i].dist         = 0;
        break;
      }
    }
  }

  // assign remaining points to closest cluster
  compute_distances(ndata, npoints, data, mask, weight, transpose, dists, metric);
  for (n = 0; n < npoints; n++) {
    if (dists[n].chosen) continue;
    clusterid[dists[n].n] = clusterid[dists[dists[n].closest].n];
  }
}

void spreadoutassign(int nclusters, int nrows, int ncolumns,
  double** data, int** mask, double weight[], int transpose,
  double (*metric)(int, double**, double**, int**, int**, const double[], int, int, int),
  int clusterid[]) {

  int i, n, chosen = 0;
  int ndata = (transpose == 0 ? ncolumns : nrows), npoints = (transpose == 0 ? nrows : ncolumns);
  clusterpoint dists[npoints];

  for (i = 0; i < npoints; i++) {
    dists[i].n      = i;
    dists[i].chosen = 0;
    dists[i].dist   = 0;
  }

  // setup 1st centroid
  n                    = 1;
  clusterid[chosen]    = 0;
  dists[chosen].chosen = 1;

  // pick k-points for k-clusters with max distance from all centers.
  chosen = npoints - 1;
  while (n < nclusters) {
    compute_distances(ndata, npoints, data, mask, weight, transpose, dists, metric);
    qsort((void*)dists, npoints, sizeof(clusterpoint), compare);

    clusterid[dists[chosen].n] = n++;
    dists[chosen].chosen       = 1;
    dists[chosen].dist         = 0;
  }

  // assign remaining points to closest cluster
  compute_distances(ndata, npoints, data, mask, weight, transpose, dists, metric);
  for (n = 0; n < npoints; n++) {
    if (dists[n].chosen) continue;
    clusterid[dists[n].n] = clusterid[dists[dists[n].closest].n];
  }
}
