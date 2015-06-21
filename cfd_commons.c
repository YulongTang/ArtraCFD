/****************************************************************************
 *                              ArtraCFD                                    *
 *                          <By Huangrui Mo>                                *
 * Copyright (C) 2014-2018 Huangrui Mo <huangrui.mo@gmail.com>              *
 * This file is part of ArtraCFD.                                           *
 * ArtraCFD is free software: you can redistribute it and/or modify it      *
 * under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 ****************************************************************************/
/****************************************************************************
 * Required Header Files
 ****************************************************************************/
#include "cfd_commons.h"
#include <stdio.h> /* standard library for input and output */
#include <math.h> /* common mathematical functions */
#include "commons.h"
/****************************************************************************
 * Function Pointers
 ****************************************************************************/
/*
 * Function pointers are useful for implementing a form of polymorphism.
 * They are mainly used to reduce or avoid switch statement. Pointers to
 * functions can get rather messy. Declaring a typedef to a function pointer
 * generally clarifies the code.
 */
typedef int (*EigenvaluesAndEigenvectorSpaceLComputer)(Real [], Real [][DIMU],
        const int, const int, const int, const Real *, const Space *, const Model *);
typedef int (*ConvectiveFluxAssembler)(Real [], const Real [], const Real [], const Real);
/****************************************************************************
 * Static Function Declarations
 ****************************************************************************/
static int AssembledFluxZ(Real [], const Real [], const Real [], const Real);
static int AssembledFluxY(Real [], const Real [], const Real [], const Real);
static int AssembledFluxX(Real [], const Real [], const Real [], const Real);
static int CalculateAlpha(Real [], Real [][DIMU], const Real []);
static int EigenvaluesAndEigenvectorSpaceLZ(
        Real [], Real [][DIMU], const int, const int, const int, 
        const Real *, const Space *, const Model *);
static int EigenvaluesAndEigenvectorSpaceLY(
        Real [], Real [][DIMU], const int, const int, const int, 
        const Real *, const Space *, const Model *);
static int EigenvaluesAndEigenvectorSpaceLX(
        Real [], Real [][DIMU], const int, const int, const int, 
        const Real *, const Space *, const Model *);
/****************************************************************************
 * Function definitions
 ****************************************************************************/
int FluxVectorSplitting(const int s, Real Fplus[], Real Fminus[], const int k, 
        const int j, const int i, const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store primitive variables rho/2gamma, u, v, w, h, c */
    PrimitiveByConservative(Uo, IndexMath(k, j, i, space) * DIMU, U, model);
    Uo[5] = sqrt(model->gamma * Uo[4] / Uo[0]); /* the speed of sound */
    Uo[4] = Uo[5] * Uo[5] / (model->gamma - 1.0); /* h */
    Uo[0] = Uo[0] / (2 * model->gamma); /* rho/2gamma */
    const int h[DIMS][DIMS] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; /* direction indicator */
    Real lambda[DIMU] = {0.0}; /* eigenvalues */
    Real lambdaPlus[DIMU] = {0.0}; /* positive part of eigenvalues */
    Real lambdaMinus[DIMU] = {0.0}; /* negative part of  eigenvalues */
    lambda[1] = h[s][Z] * Uo[3] + h[s][Y] * Uo[2] + h[s][X] * Uo[1];
    lambda[2] = lambda[1];
    lambda[3] = lambda[1];
    lambda[0] = lambda[1] - Uo[5];
    lambda[4] = lambda[1] + Uo[5];
    for (int row = 0; row < DIMU; ++row) {
        lambdaPlus[row] = 0.5 * (lambda[row] + fabs(lambda[row]));
        lambdaMinus[row] = 0.5 * (lambda[row] - fabs(lambda[row]));
    }
    ConvectiveFluxAssembler AssembleConvectiveFlux[DIMS] = {
        AssembledFluxX,
        AssembledFluxY,
        AssembledFluxZ};
    AssembleConvectiveFlux[s](Fplus, lambdaPlus, Uo, model->gamma);
    AssembleConvectiveFlux[s](Fminus, lambdaMinus, Uo, model->gamma);
}
static int AssembledFluxZ(Real F[], const Real lambda[], const Real Uo[], const Real gamma)
{
    F[0] = Uo[0] * (lambda[0] + 2 * (gamma - 1) * lambda[3] + lambda[4]);
    F[1] = F[0] * Uo[1];
    F[2] = F[0] * Uo[2];
    F[3] = F[0] * Uo[3] + Uo[0] * Uo[5] * (lambda[4] - lambda[0]);
    F[4] = F[0] * 0.5 * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]) +
        Uo[0] * (Uo[4] * (lambda[0] + lambda[4]) + Uo[3] * Uo[5] * (lambda[4] - lambda[0]));
}
static int AssembledFluxY(Real F[], const Real lambda[], const Real Uo[], const Real gamma)
{
    F[0] = Uo[0] * (lambda[0] + 2 * (gamma - 1) * lambda[2] + lambda[4]);
    F[1] = F[0] * Uo[1];
    F[2] = F[0] * Uo[2] + Uo[0] * Uo[5] * (lambda[4] - lambda[0]);
    F[3] = F[0] * Uo[3];
    F[4] = F[0] * 0.5 * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]) +
        Uo[0] * (Uo[4] * (lambda[0] + lambda[4]) + Uo[2] * Uo[5] * (lambda[4] - lambda[0]));
}
static int AssembledFluxX(Real F[], const Real lambda[], const Real Uo[], const Real gamma)
{
    F[0] = Uo[0] * (lambda[0] + 2 * (gamma - 1) * lambda[1] + lambda[4]);
    F[1] = F[0] * Uo[1] + Uo[0] * Uo[5] * (lambda[4] - lambda[0]);
    F[2] = F[0] * Uo[2];
    F[3] = F[0] * Uo[3];
    F[4] = F[0] * 0.5 * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]) +
        Uo[0] * (Uo[4] * (lambda[0] + lambda[4]) + Uo[1] * Uo[5] * (lambda[4] - lambda[0]));
}
int EigenvaluesAndDecompositionCoefficientAlpha(const int s,
        Real lambda[], Real alpha[], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    const int idx = IndexMath(k, j, i, space) * DIMU;
    const int h[DIMS][DIMS] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; /* direction indicator */
    const int idxh = IndexMath(k + h[s][Z], j + h[s][Y], i + h[s][X], space) * DIMU;;
    Real L[DIMU][DIMU] = {{0.0}}; /* store left eigenvectors */
    EigenvaluesAndEigenvectorSpaceLComputer ComputeEigenvaluesAndEigenvectorSpaceL[DIMS] = {
        EigenvaluesAndEigenvectorSpaceLX,
        EigenvaluesAndEigenvectorSpaceLY,
        EigenvaluesAndEigenvectorSpaceLZ};
    const Real deltaU[DIMU] = {
        U[idxh] - U[idx],
        U[idxh+1] - U[idx+1],
        U[idxh+2] - U[idx+2],
        U[idxh+3] - U[idx+3],
        U[idxh+4] - U[idx+4]};
    ComputeEigenvaluesAndEigenvectorSpaceL[s](lambda, L, k, j, i, U, space, model);
    CalculateAlpha(alpha, L, deltaU);
    return 0;
}
static int CalculateAlpha(Real alpha[], Real L[][DIMU], const Real deltaU[])
{
    for (int row = 0; row < DIMU; ++row) {
        alpha[row] = 0;
        for (int dummy = 0; dummy < DIMU; ++dummy) {
            alpha[row] = alpha[row] + L[row][dummy] * deltaU[dummy];
        }
    }
    return 0;
}
static int EigenvaluesAndEigenvectorSpaceLZ(
        Real lambda[], Real L[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k + 1, j, i, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    const Real b = (model->gamma - 1.0) / (2.0 * c * c);
    const Real d = (1.0 / (2.0 * c)); 
    lambda[0] = w - c; lambda[1] = w; lambda[2] = w; lambda[3] = w; lambda[4] = w + c;
    L[0][0] = b * q + d * w;   L[0][1] = -b * u;             L[0][2] = -b * v;             L[0][3] = -b * w - d;     L[0][4] = b;
    L[1][0] = -2 * b * q * u;  L[1][1] = 2 * b * u * u + 1;  L[1][2] = 2 * b * v * u;      L[1][3] = 2 * b * w * u;  L[1][4] = -2 * b * u;
    L[2][0] = -2 * b * q * v;  L[2][1] = 2 * b * v * u;      L[2][2] = 2 * b * v * v + 1;  L[2][3] = 2 * b * w * v;  L[2][4] = -2 * b * v;
    L[3][0] = -2 * b * q + 1;  L[3][1] = 2 * b * u;          L[3][2] = 2 * b * v;          L[3][3] = 2 * b * w;      L[3][4] = -2 * b;
    L[4][0] = b * q - d * w;   L[4][1] = -b * u;             L[4][2] = -b * v;             L[4][3] = -b * w + d;     L[4][4] = b;
    return 0;
}
static int EigenvaluesAndEigenvectorSpaceLY(
        Real lambda[], Real L[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k, j + 1, i, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    const Real b = (model->gamma - 1.0) / (2.0 * c * c);
    const Real d = (1.0 / (2.0 * c)); 
    lambda[0] = v - c; lambda[1] = v; lambda[2] = v; lambda[3] = v; lambda[4] = v + c;
    L[0][0] = b * q + d * v;    L[0][1] = -b * u;             L[0][2] = -b * v - d;     L[0][3] = -b * w;             L[0][4] = b;
    L[1][0] = -2 * b * q * u;   L[1][1] = 2 * b * u * u + 1;  L[1][2] = 2 * b * v * u;  L[1][3] = 2 * b * w * u;      L[1][4] = -2 * b * u;
    L[2][0] = -2 * b * q + 1;   L[2][1] = 2 * b * u;          L[2][2] = 2 * b * v;      L[2][3] = 2 * b * w;          L[2][4] = -2 * b;
    L[3][0] = -2 * b * q * w;   L[3][1] = 2 * b * w * u;      L[3][2] = 2 * b * w * v;  L[3][3] = 2 * b * w * w + 1;  L[3][4] = -2 * b * w;
    L[4][0] = b * q - d * v;    L[4][1] = -b * u;             L[4][2] = -b * v + d;     L[4][3] = -b * w;             L[4][4] = b;
    return 0;
}
static int EigenvaluesAndEigenvectorSpaceLX(
        Real lambda[], Real L[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k, j, i + 1, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    const Real b = (model->gamma - 1.0) / (2.0 * c * c);
    const Real d = (1.0 / (2.0 * c)); 
    lambda[0] = u - c; lambda[1] = u; lambda[2] = u; lambda[3] = u; lambda[4] = u + c;
    L[0][0] = b * q + d * u;   L[0][1] = -b * u - d;     L[0][2] = -b * v;             L[0][3] = -b * w;             L[0][4] = b;
    L[1][0] = -2 * b * q + 1;  L[1][1] = 2 * b * u;      L[1][2] = 2 * b * v;          L[1][3] = 2 * b * w;          L[1][4] = -2 * b;
    L[2][0] = -2 * b * q * v;  L[2][1] = 2 * b * v * u;  L[2][2] = 2 * b * v * v + 1;  L[2][3] = 2 * b * w * v;      L[2][4] = -2 * b * v;
    L[3][0] = -2 * b * q * w;  L[3][1] = 2 * b * w * u;  L[3][2] = 2 * b * w * v;      L[3][3] = 2 * b * w * w + 1;  L[3][4] = -2 * b * w;
    L[4][0] = b * q - d * u;   L[4][1] = -b * u + d;     L[4][2] = -b * v;             L[4][3] = -b * w;             L[4][4] = b;
    return 0;
}
int EigenvectorSpaceRZ(Real R[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k + 1, j, i, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real hT = Uo[4];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    R[0][0] = 1;           R[0][1] = 0;  R[0][2] = 0;  R[0][3] = 1;          R[0][4] = 1;
    R[1][0] = u;           R[1][1] = 1;  R[1][2] = 0;  R[1][3] = 0;          R[1][4] = u;
    R[2][0] = v;           R[2][1] = 0;  R[2][2] = 1;  R[2][3] = 0;          R[2][4] = v;
    R[3][0] = w - c;       R[3][1] = 0;  R[3][2] = 0;  R[3][3] = w;          R[3][4] = w + c;
    R[4][0] = hT - w * c;  R[4][1] = u;  R[4][2] = v;  R[4][3] = w * w - q;  R[4][4] = hT + w * c;
    return 0;
}
int EigenvectorSpaceRY(Real R[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k, j + 1, i, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real hT = Uo[4];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    R[0][0] = 1;           R[0][1] = 0;  R[0][2] = 1;          R[0][3] = 0;  R[0][4] = 1;
    R[1][0] = u;           R[1][1] = 1;  R[1][2] = 0;          R[1][3] = 0;  R[1][4] = u;
    R[2][0] = v - c;       R[2][1] = 0;  R[2][2] = v;          R[2][3] = 0;  R[2][4] = v + c;
    R[3][0] = w;           R[3][1] = 0;  R[3][2] = 0;          R[3][3] = 1;  R[3][4] = w;
    R[4][0] = hT - v * c;  R[4][1] = u;  R[4][2] = v * v - q;  R[4][3] = w;  R[4][4] = hT + v * c;
    return 0;
}
int EigenvectorSpaceRX(Real R[][DIMU], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    Real Uo[DIMUo] = {0.0}; /* store averaged primitive variables rho, u, v, w, hT, c */
    ComputeRoeAverage(Uo, IndexMath(k, j, i, space) * DIMU, IndexMath(k, j, i + 1, space) * DIMU, U, model);
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real hT = Uo[4];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    R[0][0] = 1;           R[0][1] = 1;          R[0][2] = 0;  R[0][3] = 0;  R[0][4] = 1;
    R[1][0] = u - c;       R[1][1] = u;          R[1][2] = 0;  R[1][3] = 0;  R[1][4] = u + c;
    R[2][0] = v;           R[2][1] = 0;          R[2][2] = 1;  R[2][3] = 0;  R[2][4] = v;
    R[3][0] = w;           R[3][1] = 0;          R[3][2] = 0;  R[3][3] = 1;  R[3][4] = w;
    R[4][0] = hT - u * c;  R[4][1] = u * u - q;  R[4][2] = v;  R[4][3] = w;  R[4][4] = hT + u * c;
    return 0;
}
int ComputeRoeAverage(Real Uo[], const int idx, const int idxh, const Real *U, const Model *model)
{
    const Real gamma = model->gamma;
    const Real rho = U[idx];
    const Real u = U[idx+1] / rho;
    const Real v = U[idx+2] / rho;
    const Real w = U[idx+3] / rho;
    const Real hT = (U[idx+4] / rho) * gamma - 0.5 * (u * u + v * v + w * w) * (gamma - 1.0);
    const Real rho_h = U[idxh];
    const Real u_h = U[idxh+1] / rho_h;
    const Real v_h = U[idxh+2] / rho_h;
    const Real w_h = U[idxh+3] / rho_h;
    const Real hT_h = (U[idxh+4] / rho_h) * gamma - 0.5 * (u_h * u_h + v_h * v_h + w_h * w_h) * (gamma - 1.0);
    const Real D = sqrt(rho_h / rho);
    //Uo[0] = rho * (0.5 * (1.0 + D)) * (0.5 * (1.0 + D)); /* rho average, not required */
    Uo[1] = (u + D * u_h) / (1.0 + D); /* u average */
    Uo[2] = (v + D * v_h) / (1.0 + D); /* v average */
    Uo[3] = (w + D * w_h) / (1.0 + D); /* w average */
    Uo[4] = (hT + D * hT_h) / (1.0 + D); /* hT average */
    Uo[5] = sqrt((gamma - 1.0) * (Uo[4] - 0.5 * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]))); /* the speed of sound */
    return 0;
}
int ConvectiveFluxZ(Real F[], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    const int idx = IndexMath(k, j, i, space) * DIMU;
    const Real rho = U[idx];
    const Real u = U[idx+1] / rho;
    const Real v = U[idx+2] / rho;
    const Real w = U[idx+3] / rho;
    const Real eT = U[idx+4] / rho;
    const Real p = rho * (eT - 0.5 * (u * u + v * v + w * w)) * (model->gamma - 1.0);
    F[0] = rho * w;
    F[1] = rho * w * u;
    F[2] = rho * w * v;
    F[3] = rho * w * w + p;
    F[4] = (rho * eT + p) * w;
    return 0;
}
int ConvectiveFluxY(Real F[], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    const int idx = IndexMath(k, j, i, space) * DIMU;
    const Real rho = U[idx];
    const Real u = U[idx+1] / rho;
    const Real v = U[idx+2] / rho;
    const Real w = U[idx+3] / rho;
    const Real eT = U[idx+4] / rho;
    const Real p = rho * (eT - 0.5 * (u * u + v * v + w * w)) * (model->gamma - 1.0);
    F[0] = rho * v;
    F[1] = rho * v * u;
    F[2] = rho * v * v + p;
    F[3] = rho * v * w;
    F[4] = (rho * eT + p) * v;
    return 0;
}
int ConvectiveFluxX(Real F[], const int k, const int j, const int i, 
        const Real *U, const Space *space, const Model *model)
{
    const int idx = IndexMath(k, j, i, space) * DIMU;
    const Real rho = U[idx];
    const Real u = U[idx+1] / rho;
    const Real v = U[idx+2] / rho;
    const Real w = U[idx+3] / rho;
    const Real eT = U[idx+4] / rho;
    const Real p = rho * (eT - 0.5 * (u * u + v * v + w * w)) * (model->gamma - 1.0);
    F[0] = rho * u;
    F[1] = rho * u * u + p;
    F[2] = rho * u * v;
    F[3] = rho * u * w;
    F[4] = (rho * eT + p) * u;
    return 0;
}
/*
 * Get value of primitive variable vector.
 */
int PrimitiveByConservative(Real Uo[], const int idx, const Real *U, const Model *model)
{
    Uo[0] = U[idx];
    Uo[1] = U[idx+1] / U[idx];
    Uo[2] = U[idx+2] / U[idx];
    Uo[3] = U[idx+3] / U[idx];
    Uo[4] = (U[idx+4] - 0.5 * (U[idx+1] * U[idx+1] + U[idx+2] * U[idx+2] + U[idx+3] * U[idx+3]) / U[idx]) * (model->gamma - 1.0);
    Uo[5] = Uo[4] / (Uo[0] * model->gasR);
    return 0;
}
/*
 * Compute conservative variable vector according to primitives.
 */
int ConservativeByPrimitive(Real *U, const int idx, const Real Uo[], const Model *model)
{
    U[idx] = Uo[0];
    U[idx+1] = Uo[0] * Uo[1];
    U[idx+2] = Uo[0] * Uo[2];
    U[idx+3] = Uo[0] * Uo[3];
    U[idx+4] = 0.5 * Uo[0] * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]) + Uo[4] / (model->gamma - 1.0); 
    return 0;
}
Real ComputePressure(const int idx, const Real *U, const Model *model)
{
    return (U[idx+4] - 0.5 * (U[idx+1] * U[idx+1] + U[idx+2] * U[idx+2] + U[idx+3] * U[idx+3]) / U[idx]) * (model->gamma - 1.0);
}
Real ComputeTemperature(const int idx, const Real *U, const Model *model)
{
    return (U[idx+4] - 0.5 * (U[idx+1] * U[idx+1] + U[idx+2] * U[idx+2] + U[idx+3] * U[idx+3]) / U[idx]) / (U[idx] * model->cv);
}
/*
 * Index math.
 */
int IndexMath(const int k, const int j, const int i, const Space *space)
{
    return ((k * space->jMax + j) * space->iMax + i);
}
Real *IndexGeometry(const int geoID, const Geometry *geometry)
{
    return geometry->headAddress + geoID * ENTRYGEO;
}
/*
 * Coordinates transformations.
 * When transform from spatial coordinates to node coordinates, a half
 * grid distance shift is necessary to ensure obtaining a closest node
 * coordinates considering the downward truncation of (int). 
 * Note: current rounding conversion only works for positive float.
 */
int ComputeK(const Real z, const Space *space)
{
    return (int)((z - space->zMin) * space->ddz + 0.5) + space->ng;
}
int ComputeJ(const Real y, const Space *space)
{
    return (int)((y - space->yMin) * space->ddy + 0.5) + space->ng;
}
int ComputeI(const Real x, const Space *space)
{
    return (int)((x - space->xMin) * space->ddx + 0.5) + space->ng;
}
int ValidRegionK(const int k, const Partition *part)
{
    return MinInt(part->kSup[0] - 1, MaxInt(part->kSub[0], k));
}
int ValidRegionJ(const int j, const Partition *part)
{
    return MinInt(part->jSup[0] - 1, MaxInt(part->jSub[0], j));
}
int ValidRegionI(const int i, const Partition *part)
{
    return MinInt(part->iSup[0] - 1, MaxInt(part->iSub[0], i));
}
Real ComputeZ(const int k, const Space *space)
{
    return (space->zMin + (k - space->ng) * space->dz);
}
Real ComputeY(const int j, const Space *space)
{
    return (space->yMin + (j - space->ng) * space->dy);
}
Real ComputeX(const int i, const Space *space)
{
    return (space->xMin + (i - space->ng) * space->dx);
}
Real MinReal(const Real x, const Real y)
{
    if (x < y) {
        return x;
    }
    return y;
}
Real MaxReal(const Real x, const Real y)
{
    if (x > y) {
        return x;
    }
    return y;
}
int MinInt(const int x, const int y)
{
    if (x < y) {
        return x;
    }
    return y;
}
int MaxInt(const int x, const int y)
{
    if (x > y) {
        return x;
    }
    return y;
}
int Sign(const Real x)
{
    if (0 < x) {
        return 1;
    }
    if (0 > x) {
        return -1;
    }
    return 0;
}
/* a good practice: end file with a newline */

