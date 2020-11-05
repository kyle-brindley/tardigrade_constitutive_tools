/**
  * \file test_constitutive_tools.cpp
  *
  * Tests for constitutive_tools
  */

#include<constitutive_tools.h>
#include<sstream>
#include<fstream>
#include<iostream>

#define BOOST_TEST_MODULE test_vector_tools
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

typedef constitutiveTools::errorOut errorOut;
typedef constitutiveTools::floatType floatType;
typedef constitutiveTools::floatVector floatVector;
typedef constitutiveTools::floatMatrix floatMatrix;

struct cout_redirect{
    cout_redirect( std::streambuf * new_buffer)
        : old( std::cout.rdbuf( new_buffer ) )
    { }

    ~cout_redirect( ) {
        std::cout.rdbuf( old );
    }

    private:
        std::streambuf * old;
};

struct cerr_redirect{
    cerr_redirect( std::streambuf * new_buffer)
        : old( std::cerr.rdbuf( new_buffer ) )
    { }

    ~cerr_redirect( ) {
        std::cerr.rdbuf( old );
    }

    private:
        std::streambuf * old;
};

BOOST_AUTO_TEST_CASE( testDeltaDirac ){
    /*!
     * Test the deltaDirac function in constitutive tools
     * 
     * :param std::ofstream &results: The output file
     */

    if (constitutiveTools::deltaDirac(1, 2) != 0){
        results << "deltaDirac & False\n";
        return 1;
    }

    if (constitutiveTools::deltaDirac(1, 1) != 1){
        results << "deltaDirac & False\n";
        return 1;
    }

    results << "deltaDirac & True\n";
    return 0;
    
}

BOOST_AUTO_TEST_CASE( testRotateMatrix ){
    /*!
     * Test the rotation of a matrix by an orthogonal rotation matrix..
     * 
     * :param std::ofstream &results: The output file
     */


    floatVector Q = {-0.44956296, -0.88488713, -0.12193405,
                     -0.37866166,  0.31242661, -0.87120891,
                      0.80901699, -0.3454915 , -0.47552826};

    floatVector A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    floatVector rotatedA;
    errorOut ret = constitutiveTools::rotateMatrix(A, Q, rotatedA);

    if (ret){
        results << "testRotatedMatrix (test 1) & False\n";
        return 1;
    }

    if (! vectorTools::fuzzyEquals( rotatedA, {-0.09485264, -3.38815017, -5.39748037,
                                               -1.09823916,  2.23262233,  4.68884658,
                                               -1.68701666,  6.92240128, 12.8622303})){
        results << "testRotatedMatrix (test 1) & False\n";
        return 1;
    }

    //Test rotation back to original frame
    
    floatVector QT(Q.size(), 0);
    for (unsigned int i=0; i<3; i++){
        for (unsigned int j=0; j<3; j++){
            QT[3*j + i] = Q[3*i + j];
        }
    }

    floatVector App;
    ret = constitutiveTools::rotateMatrix(rotatedA, QT, App);

    if (ret){
        results << "testRotateMatrix (test 2) & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A, App) );

    results << "testRotatedMatrix & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeGreenLagrangeStrain ){
    /*!
     * Test the computation of the Green-Lagrange strain
     * 
     * :param std::ofstream &results: The output file
     */

    floatVector F = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    floatVector E;

    constitutiveTools::errorOut ret;
    ret = constitutiveTools::computeGreenLagrangeStrain(F, E);

    if (ret){
        results << "testComputeGreenLagrangeStrain (test 1) & False\n";
        return 1;
    }
    
    BOOST_CHECK( vectorTools::fuzzyEquals(E, {0, 0, 0, 0, 0, 0, 0, 0, 0}) );

    F = {0.69646919, 0.28613933, 0.22685145,
         0.55131477, 0.71946897, 0.42310646,
         0.98076420, 0.68482974, 0.4809319};

    ret = constitutiveTools::computeGreenLagrangeStrain(F, E);

    if (ret){
        results << "testComputeGreenLagrangeStrain (test 2) & False\n";
        return 1;
    }

    if (! vectorTools::fuzzyEquals(E, {0.37545786,  0.63379879,  0.43147034,
                                       0.63379879,  0.03425154,  0.34933978,
                                       0.43147034,  0.34933978, -0.26911192})){
        results << "testComputeGreenLagrangeStrain (test 2) & False\n";
        return 1;
    }

    floatVector EJ;
    floatMatrix dEdF;
    ret = constitutiveTools::computeGreenLagrangeStrain(F, EJ, dEdF);

    if (ret){
        results << "testComputeGreenLagrangeStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(E, EJ) );

    floatType eps = 1e-6;
    for (unsigned int i=0; i<F.size(); i++){
        floatVector delta(F.size(), 0);

        delta[i] = eps * fabs(F[i]) + eps;

        ret = constitutiveTools::computeGreenLagrangeStrain(F + delta, EJ);

        if (ret){
            results << "testComputeGreenLagrangeStrain & False\n";
            return 1;
        }

        floatVector gradCol = (EJ - E)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dEdF[j][i]) );
        }
    }

    results << "testComputeGreenLagrangeStrain & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testDecomposeGreenLagrangeStrain ){
    /*!
     * Test the decomposition of the Green-Lagrange strain into isochoric and 
     * volumetric parts.
     */

    floatVector F = {0.69646919, 0.28613933, 0.22685145,
                     0.55131477, 0.71946897, 0.42310646,
                     0.98076420, 0.68482974, 0.4809319};

    floatType J = vectorTools::determinant(F, 3, 3);
    floatVector Fbar = F/pow(J, 1./3);

    floatVector E, Ebar;
    errorOut ret = constitutiveTools::computeGreenLagrangeStrain(Fbar, Ebar);

    if (ret){
        ret->print();
        results << "testDecomposeGreenLagrangeStrain & False\n";
        return 1;
    }

    ret = constitutiveTools::computeGreenLagrangeStrain(F, E);

    if (ret){
        ret->print();
        results << "testDecomposeGreenLagrangeStrain & False\n";
        return 1;
    }

    floatType JOut;
    floatVector EbarOut;
    ret = constitutiveTools::decomposeGreenLagrangeStrain(E, EbarOut, JOut);

    if (ret){
        ret->print();
        results << "testDecomposeGreenLagrangeStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(J, JOut) );

    BOOST_CHECK( vectorTools::fuzzyEquals(EbarOut, Ebar) );

    floatVector EbarOut2;
    floatType JOut2;
    floatMatrix dEbardE;
    floatVector dJdE;
    ret = constitutiveTools::decomposeGreenLagrangeStrain(E, EbarOut2, JOut2, dEbardE, dJdE);

    if (ret){
        ret->print();
        results << "testDecomposeGreenLagrangeStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(EbarOut, EbarOut2) );

    BOOST_CHECK( vectorTools::fuzzyEquals(JOut, JOut2) );

    floatType eps = 1e-8;
    for (unsigned int i=0; i<E.size(); i++){
        floatVector delta(E.size(), 0);
        delta[i] =  fabs(eps*E[i]);
     
        ret = constitutiveTools::decomposeGreenLagrangeStrain(E + delta, EbarOut2, JOut2);

        if (ret){
            ret->print();
            results << "testDecomposeGreenLagrangeStrain (test 5) & False\n";
            return 1;
        }

        BOOST_CHECK( vectorTools::fuzzyEquals((JOut2 - JOut)/delta[i], dJdE[i], 1e-4, 1e-4) );
    }

    for (unsigned int i=0; i<E.size(); i++){
        floatVector delta(E.size(), 0);
        delta[i] = fabs(eps*E[i]);
     
        ret = constitutiveTools::decomposeGreenLagrangeStrain(E + delta, EbarOut2, JOut2);

        if (ret){
            ret->print();
            results << "testDecomposeGreenLagrangeStrain (test 5) & False\n";
            return 1;
        }

        floatVector gradCol = (EbarOut2 - EbarOut)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dEbardE[j][i], 1e-4, 1e-4) );
        }
    }

    floatVector badE = {-1, 0, 0, 0, 1, 0, 0, 0, 1};

    ret = constitutiveTools::decomposeGreenLagrangeStrain(badE, EbarOut, JOut);

    BOOST_CHECK( ret );

    results << "testDecomposeGreenLagrangeStrain & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testMapPK2toCauchy ){
    /*!
     * Test the mapping of the PK2 stress from the reference 
     * configuration to the current configuration.
     * 
     * :param std::ofstream &results: The output file
     */
    
    floatVector F = {1.96469186, -2.13860665, -2.73148546,
                     0.51314769,  2.1946897,  -0.7689354,
                     4.80764198,  1.84829739, -0.19068099};

    floatVector PK2 = {-1.07882482, -1.56821984,  2.29049707,
                       -0.61427755, -4.40322103, -1.01955745,
                        2.37995406, -3.1750827,  -3.24548244};

    floatVector cauchy;

    errorOut error = constitutiveTools::mapPK2toCauchy(PK2, F, cauchy);

    if (error){
        error->print();
        results << "testMapPK2toCauchy & False\n";
        return 1;
    }

    if (!vectorTools::fuzzyEquals(cauchy, {-2.47696057,  0.48015011, -0.28838671,
                                            0.16490963, -0.57481137, -0.92071407,
                                           -0.21450698, -1.22714923, -1.73532173})){
        results << "testMapPK2toCauchy (test 1) & False\n";
        return 1;
    }

    results << "testMapPK2toCauchy & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testWLF ){
    /*!
     * Test the computation of the WLF function.
     * 
     * :param std::ofstream &results: The output file
     */

    floatType T = 145.;
    floatType Tr = 27.5;
    floatType C1 = 18.2;
    floatType C2 = 282.7;
    
    floatType factor, dfactordT;

    floatVector WLFParameters {Tr, C1, C2};

    constitutiveTools::WLF(T, WLFParameters, factor);

    BOOST_CHECK( vectorTools::fuzzyEquals(factor, pow(10, -C1*(T - Tr)/(C2 + (T - Tr)))) );

    floatType factor2;
    constitutiveTools::WLF(T, WLFParameters, factor2, dfactordT);
    
    BOOST_CHECK( vectorTools::fuzzyEquals(factor, factor2) );

    floatType delta = fabs(1e-6*T);
    constitutiveTools::WLF(T + delta, WLFParameters, factor2);

    BOOST_CHECK( vectorTools::fuzzyEquals(dfactordT, (factor2 - factor)/delta) );

    results << "testWLF & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeDGreenLagrangeStrainDF ){
    /*!
     * Test the computation of the gradient of the Green-Lagrange 
     * strain w.r.t. the deformation gradient.
     * 
     * :param std::ofstream &results: The output file.
     */

    floatVector F = {0.69646919, 0.28613933, 0.22685145,
                     0.55131477, 0.71946897, 0.42310646,
                     0.98076420, 0.68482974, 0.4809319};

    floatMatrix dEdF;

    errorOut error = constitutiveTools::computeDGreenLagrangeStrainDF(F, dEdF);

    if (error){
        error->print();
        results << "testComputeDGreenLagrangeStrainDF & False\n";
        return 1;
    }

    floatVector E, E2;
    error = constitutiveTools::computeGreenLagrangeStrain(F, E);

    if (error){
        error->print();
        results << "testComputeDGreenLagrangeStrainDF & False\n";
        return 1;
    }

    floatType eps = 1e-6;
    for (unsigned int i=0; i<F.size(); i++){
        floatVector delta(F.size(), 0);

        delta[i] = fabs(eps*F[i]);

        error = constitutiveTools::computeGreenLagrangeStrain(F + delta, E2);

        floatVector gradCol = (E2 - E)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dEdF[j][i]) );
        }
    }
    results << "testComputeDGreenLagrangeStrainDF & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testMidpointEvolution ){
    /*!
     * Test the midpoint evolution algorithm.
     * 
     * :param std::ofstream &results: The output file
     */

    floatType Dt = 2.5;
    floatVector Ap    = {9, 10, 11, 12};
    floatVector DApDt = {1, 2, 3, 4};
    floatVector DADt  = {5, 6, 7, 8};
    floatVector alphaVec = {0.1, 0.2, 0.3, 0.4};
    floatVector A;

    //Test implicit integration
    errorOut error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A, 0);

    if (error){
        error->print();
        results << "testMidpointEvolution & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A, Ap + Dt*DADt) );

    //Test explicit integration
    error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A, 1);

    if (error){
        error->print();
        results << "testMidpointEvolution & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A, Ap + Dt*DApDt) );

    //Test midpoint integration
    error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A);

    if (error){
        error->print();
        results << "testMidpointEvolution & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A, Ap + Dt*0.5*(DApDt + DADt)) );

    error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A, alphaVec);

    if (error){
        error->print();
        results << "testMidpointEvolution & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A, {20.5, 23. , 25.5, 28.}) );

    //Add test for the jacobian
    floatType eps = 1e-6;
    floatVector A0, Ai;
    floatMatrix DADADt;

    error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A, alphaVec);
    error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt, A0, DADADt, alphaVec);

    if (error){
        error->print();
        results << "testMidpointEvolution & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(A0, A) );

    for (unsigned int i=0; i<DADt.size(); i++){
        floatVector delta = floatVector(DADt.size(), 0);
        delta[i] = eps*(DADt[i]) + eps;

        error = constitutiveTools::midpointEvolution(Dt, Ap, DApDt, DADt + delta, Ai, alphaVec);

        floatVector gradCol = (Ai - A0)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(DADADt[j][i], gradCol[j]) );
        }
        
    }
    
    results << "testMidpointEvolution & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeDFDt ){
    /*!
     * Test the computation of the total time derivative of the 
     * deformation gradient.
     * 
     * :param std::ofstream &results: The output file
     */

    floatVector F = {0.69646919, 0.28613933, 0.22685145,
                     0.55131477, 0.71946897, 0.42310646,
                     0.98076420, 0.68482974, 0.4809319};

    floatVector L = {0.57821272, 0.27720263, 0.45555826,
                     0.82144027, 0.83961342, 0.95322334,
                     0.4768852 , 0.93771539, 0.1056616};

    floatVector answer = {1.00232848, 0.67686793, 0.46754712,
                          1.96988645, 1.49191786, 1.00002629,
                          0.95274131, 0.88347295, 0.55575157};

    floatVector DFDt;

    errorOut error = constitutiveTools::computeDFDt(L, F, DFDt);

    if (error){
        error->print();
        results << "testComputeDFDt & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(DFDt, answer) );

    //Test the jacobians
    floatVector DFDtJ;
    floatMatrix dDFDtdL, dDFDtdF;
    error = constitutiveTools::computeDFDt(L, F, DFDtJ, dDFDtdL, dDFDtdF);

    if (error){
        error->print();
        results << "testComputeDFDt & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(DFDt, DFDtJ) );

    //Use finite differences to estimate the jacobian
    floatType eps = 1e-6;
    for (unsigned int i=0; i<F.size(); i++){

        //Compute finite difference gradient w.r.t. L
        floatVector delta(L.size(), 0);
        delta[i] = eps*fabs(L[i]) + eps;

        error = constitutiveTools::computeDFDt(L + delta, F, DFDtJ);

        if (error){
            error->print();
            results << "testComputeDFDt & False\n";
            return 1;
        }

        floatVector gradCol = (DFDtJ - DFDt)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(dDFDtdL[j][i], gradCol[j]) );
        }

        //Compute finite difference gradient w.r.t. F
        delta = floatVector(F.size(), 0);
        delta[i] = eps*fabs(F[i]) + eps;

        error = constitutiveTools::computeDFDt(L, F + delta, DFDtJ);

        if (error){
            error->print();
            results << "testComputeDFDt & False\n";
            return 1;
        }

        gradCol = (DFDtJ - DFDt)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(dDFDtdF[j][i], gradCol[j]) );
        }


    }

    results << "testComputeDFDt & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testEvolveF ){
    /*!
     * Test the evolution of the deformation gradient.
     * 
     * :param std::ofstream &results: The output file
     */
    
    floatType Dt = 2.7;

    floatVector Fp = {0.69646919, 0.28613933, 0.22685145,
                      0.55131477, 0.71946897, 0.42310646,
                      0.98076420, 0.68482974, 0.4809319};

    floatVector Lp = {0.69006282, 0.0462321 , 0.88086378,
                      0.8153887 , 0.54987134, 0.72085876, 
                      0.66559485, 0.63708462, 0.54378588};

    floatVector L = {0.57821272, 0.27720263, 0.45555826,
                     0.82144027, 0.83961342, 0.95322334,
                     0.4768852 , 0.93771539, 0.1056616};

    //Test 1 (mode 1 fully explicit)
    floatVector F;
    errorOut error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 1, 1);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    floatVector answer = {4.39551129, 2.53782698, 1.84614498,
                          4.81201673, 3.75047725, 2.48674399,
                          4.62070491, 3.44211354, 2.32252023};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Test 2 (mode 1 fully implicit)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 0, 1);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    answer = {0.63522182, -0.1712192 , -0.00846781,
             -0.81250979, -0.19375022, -0.20193394,
             -0.36163914, -0.03662069, -0.05769288};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Test 3 (mode 1 midpoint rule)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 0.5, 1);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    answer = {0.20004929, -0.4409338 , -0.18955924,
             -3.59005736, -2.17210401, -1.55661536,
             -1.88391214, -1.13150095, -0.80579654};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Tests 4 and 5 (mode 1 jacobian)
    floatVector FJ;
    floatMatrix dFdL;
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, FJ, dFdL, 0.5, 1);

    if (error){
        error->print(); 
        results << "testEvolveF & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(F, FJ) );

    floatType eps = 1e-6;
    for (unsigned int i=0; i<L.size(); i++){

        floatVector delta(L.size(), 0);
        delta[i] = eps*fabs(L[i]) + eps;

        error = constitutiveTools::evolveF(Dt, Fp, Lp, L + delta, FJ, 0.5, 1);

        if (error){
            error->print();
            results << "testEvolveF & False\n";
            return 1;
        }

        floatVector gradCol = (FJ - F)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dFdL[j][i], 1e-5, 1e-5) );
        }

    }

    //Test 6 (mode 2 fully explicit)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 1, 2);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    answer = {3.03173544, 1.1881084 , 2.77327313,
              3.92282144, 2.58424672, 3.75584617,
              5.18006647, 2.65125419, 4.85252662};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Test 7 (mode 2 fully implicit)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 0, 2);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    answer = {0.65045472, -0.42475879, -0.09274688,
             -0.25411831, -0.08867872, -0.16467241,
              0.45611733, -0.45427799, -0.17799727};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Test 8 (mode 2 midpoint rule)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, F, 0.5, 2);

    if (error){
        error->print();
        results << "testEvolveF & False\n";
        return 1;
    }

    answer = {-0.02066217, -1.43862233, -0.42448874,
              -0.96426544, -1.72139966, -0.83831629,
              -0.59802055, -2.37943476, -0.88998505};

    BOOST_CHECK( vectorTools::fuzzyEquals(answer, F) );

    //Tests 9 and 10 (mode 2 jacobian)
    error = constitutiveTools::evolveF(Dt, Fp, Lp, L, FJ, dFdL, 0.5, 2);

    if (error){
        error->print(); 
        results << "testEvolveF & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(F, FJ) );

    for (unsigned int i=0; i<L.size(); i++){

        floatVector delta(L.size(), 0);
        delta[i] = eps*fabs(L[i]) + eps;

        error = constitutiveTools::evolveF(Dt, Fp, Lp, L + delta, FJ, 0.5, 2);

        if (error){
            error->print();
            results << "testEvolveF & False\n";
            return 1;
        }

        floatVector gradCol = (FJ - F)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dFdL[j][i], 1e-5, 1e-5) );
        }

    }
    
    results << "testEvolveF & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testMac ){
    /*!
     * Test the computation of the Macullay brackets.
     * 
     * :param std::ofstream &results: The output file
     */

    floatType x = 1;
    BOOST_CHECK( vectorTools::fuzzyEquals(constitutiveTools::mac(x), x) );

    x = -1;
    BOOST_CHECK( vectorTools::fuzzyEquals(constitutiveTools::mac(x), 0.) );

    floatType xJ = 2;
    floatType dmacdx;
    BOOST_CHECK( vectorTools::fuzzyEquals(constitutiveTools::mac(xJ), constitutiveTools::mac(xJ, dmacdx)) );

    BOOST_CHECK( vectorTools::fuzzyEquals(dmacdx, 1.) );

    xJ = -2;
    BOOST_CHECK( vectorTools::fuzzyEquals(constitutiveTools::mac(xJ), constitutiveTools::mac(xJ, dmacdx)) );

    BOOST_CHECK( vectorTools::fuzzyEquals(dmacdx, 0.) );

    results << "testMac & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeUnitNormal ){
    /*!
     * Test the computation of the unit normal.
     * 
     * :param std::ofstream &results: The output file
     */

    floatVector A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    floatVector Anorm;

    errorOut error = constitutiveTools::computeUnitNormal(A, Anorm);

    if (error){
        error->print();
        results << "testComputeUnitNormal & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(vectorTools::inner(Anorm, Anorm), 1.) );

    //Check the jacobian
    floatVector AnormJ;
    floatMatrix dAnormdA;

    error = constitutiveTools::computeUnitNormal(A, AnormJ, dAnormdA);

    if (error){
        error->print();
        results << "testComputeUnitNormal & False\n";
        return 1;
    }

    //Check the normalized value
    BOOST_CHECK( vectorTools::fuzzyEquals(AnormJ, Anorm) );

    //Check the gradient
    floatType eps = 1e-6;
    for (unsigned int i=0; i<A.size(); i++){
        floatVector delta(A.size(), 0);
        delta[i] = eps*fabs(A[i]) + eps;

        error = constitutiveTools::computeUnitNormal(A + delta, AnormJ, dAnormdA);

        if (error){
            error->print();
            results << "testComputeUnitNormal & False\n";
            return 1;
        }

        floatVector gradCol = (AnormJ - Anorm)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(dAnormdA[j][i], gradCol[j]) );
        }
    }

    A = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    error = constitutiveTools::computeUnitNormal( A, Anorm );

    if ( error ){
        error->print();
        results << "testComputeUnitNormal & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals( Anorm, A )  );

    error = constitutiveTools::computeUnitNormal( A, Anorm, dAnormdA );

    if ( error ){
        error->print();
        results << "testComputeUnitNormal & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals( Anorm, A )  );

    BOOST_CHECK( std::isnan( vectorTools::l2norm( dAnormdA ) )  );

    results << "testComputeUnitNormal & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testPullBackVelocityGradient ){
    /*!
     * Test the pull back operation on the velocity gradient.
     * 
     * :param std::ofstream &results: The output file.
     */

    floatVector velocityGradient = {0.69006282, 0.0462321 , 0.88086378,
                                    0.8153887 , 0.54987134, 0.72085876,
                                    0.66559485, 0.63708462, 0.54378588};

    floatVector deformationGradient = {0.69646919, 0.28613933, 0.22685145,
                                       0.55131477, 0.71946897, 0.42310646,
                                       0.98076420, 0.68482974, 0.4809319};

    floatVector pullBackL;    
    floatVector expectedPullBackL = {6.32482111,   3.11877752,   2.43195977,
                                    20.19439192,  10.22175689,   7.88052809,
                                   -38.85113898, -18.79212468, -14.76285795};

    errorOut error = constitutiveTools::pullBackVelocityGradient(velocityGradient, deformationGradient, pullBackL);

    if (error){
        error->print();
        results << "testPullBackVelocityGradient & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(pullBackL, expectedPullBackL) );

    floatVector pullBackLJ;
    floatMatrix dpbLdL, dpbLdF;
    
    //Test of the jacobian
    error = constitutiveTools::pullBackVelocityGradient(velocityGradient, deformationGradient, pullBackLJ, 
                                                        dpbLdL, dpbLdF);

    if (error){
        error->print();
        results << "testPullBackVelocityGradient & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(pullBackL, pullBackLJ) );

    //Check dpbLdL
    floatType eps = 1e-6;
    for (unsigned int i=0; i<velocityGradient.size(); i++){
        floatVector delta(velocityGradient.size(), 0);
        delta[i] = eps*fabs(velocityGradient[i]) + eps;

        error = constitutiveTools::pullBackVelocityGradient(velocityGradient + delta, deformationGradient, pullBackLJ);

        if (error){
            error->print();
            results << "testPullBackVelocityGradient & False\n";
            return 1;
        }

        floatVector gradCol = (pullBackLJ - pullBackL)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dpbLdL[j][i]) );
        }
    }

    //Check dpbLdF
    for (unsigned int i=0; i<deformationGradient.size(); i++){
        floatVector delta(deformationGradient.size(), 0);
        delta[i] = eps*fabs(deformationGradient[i]) + eps;

        error = constitutiveTools::pullBackVelocityGradient(velocityGradient, deformationGradient + delta, pullBackLJ);

        if (error){
            error->print();
            results << "testPullBackVelocityGradient & False\n";
            return 1;
        }

        floatVector gradCol = (pullBackLJ - pullBackL)/delta[i];

        for (unsigned int j=0; j<gradCol.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(gradCol[j], dpbLdF[j][i], 1e-4) );
        }
    }

    results << "testPullBackVelocityGradient & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testQuadraticThermalExpansion ){
    /*!
     * Test the computation of the thermal expansion using a 
     * quadratic form.
     * 
     * :param std::ofstream &results: The output file.
     */

    floatType temperature = 283.15;
    floatType referenceTemperature = 273.15;

    floatVector linearParameters = {1, 2, 3, 4};
    floatVector quadraticParameters = {5, 6, 7, 8};

    floatVector thermalExpansion;
    errorOut error = constitutiveTools::quadraticThermalExpansion(     temperature, referenceTemperature, 
                                                                  linearParameters,  quadraticParameters, 
                                                                  thermalExpansion);

    if (error){
        error->print();
        results << "testQuadraticThermalExpansion & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(thermalExpansion, {510., 620., 730., 840.}) );

    floatVector thermalExpansionJ, thermalExpansionJacobian;
    floatType eps = 1e-6;
    floatType delta = eps*temperature + eps;

    error = constitutiveTools::quadraticThermalExpansion(      temperature,     referenceTemperature,
                                                          linearParameters,      quadraticParameters,
                                                         thermalExpansionJ, thermalExpansionJacobian);

    if (error){
        error->print();
        results << "testQuadraticThermalExpansion & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(thermalExpansion, thermalExpansionJ) );

    error = constitutiveTools::quadraticThermalExpansion(      temperature + delta,     referenceTemperature,
                                                                  linearParameters,      quadraticParameters,
                                                                 thermalExpansionJ);

    if (error){
        error->print();
        results << "testQuadraticThermalExpansion & False\n";
        return 1;
    }
    
    BOOST_CHECK( vectorTools::fuzzyEquals(thermalExpansionJacobian, (thermalExpansionJ - thermalExpansion)/delta, 1e-4) );

    results << "testQuadraticThermalExpansion & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testPushForwardGreenLagrangeStrain ){
    /*!
     * Test the push-forward operation on the Green-Lagrange strain.
     * 
     * :param std::ofstream &results: The output file.
     */

    floatVector deformationGradient = {0.30027935, -0.72811411,  0.26475099,
                                       1.2285819 ,  0.57663593,  1.43113814,
                                      -0.45871432,  0.2175795 ,  0.54013937};

    floatVector greenLagrangeStrain;
    errorOut error = constitutiveTools::computeGreenLagrangeStrain(deformationGradient, greenLagrangeStrain);

    if (error){
        error->print();
        results << "testPushForwardGreenLagrangeStrain & False\n";
        return 1;
    }

    floatVector almansiStrain = {-0.33393717,  0.0953188 , -0.29053383,
                                  0.0953188 ,  0.35345526,  0.11588247,
                                 -0.29053383,  0.11588247, -0.56150741};

    floatVector result;
    error = constitutiveTools::pushForwardGreenLagrangeStrain(greenLagrangeStrain, deformationGradient, 
                                                              result);

    if (error){
        error->print();
        results << "testPushForwardGreenLagrangeStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(result, almansiStrain) );

    //Test the jacobian
    floatVector resultJ;
    floatMatrix dedE, dedF;
    error = constitutiveTools::pushForwardGreenLagrangeStrain(greenLagrangeStrain, deformationGradient, 
                                                              resultJ, dedE, dedF);

    if (error){
        error->print();
        results << "testPushForwardGreenLagrangeStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals(result, resultJ) );

    //Check dedE
    floatType eps = 1e-6;
    for (unsigned int i=0; i<greenLagrangeStrain.size(); i++){
        floatVector delta(greenLagrangeStrain.size(), 0);
        delta[i] = eps*fabs(greenLagrangeStrain[i]) + eps;

        error = constitutiveTools::pushForwardGreenLagrangeStrain(greenLagrangeStrain + delta, deformationGradient, 
                                                                  resultJ);
    
        if (error){
            error->print();
            results << "testPushForwardGreenLagrangeStrain & False\n";
            return 1;
        }

        floatVector grad = (resultJ - result)/delta[i];

        for (unsigned int j=0; j<grad.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(grad[j], dedE[j][i]) );
        }
    }

    //Check dedF
    for (unsigned int i=0; i<deformationGradient.size(); i++){
        floatVector delta(deformationGradient.size(), 0);
        delta[i] = eps*fabs(deformationGradient[i]) + eps;

        error = constitutiveTools::pushForwardGreenLagrangeStrain(greenLagrangeStrain, deformationGradient + delta, 
                                                                  resultJ);
    
        if (error){
            error->print();
            results << "testPushForwardGreenLagrangeStrain & False\n";
            return 1;
        }

        floatVector grad = (resultJ - result)/delta[i];

        for (unsigned int j=0; j<grad.size(); j++){
            BOOST_CHECK( vectorTools::fuzzyEquals(grad[j], dedF[j][i], 1e-5) );
        }
    }

    results << "testPushForwardGreenLagrangeStrain & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testPullBackAlmansiStrain ){
    /*!
     * Test the pull-back operation on the Green-Lagrange strain.
     * 
     * :param std::ofstream &results: The output file.
     */

    floatVector deformationGradient = { 0.1740535 ,  1.2519364 , -0.9531442 ,
                                       -0.7512021 , -0.60229072,  0.32640812,
                                       -0.59754476, -0.06209685, -1.50856757 };

    floatVector almansiStrain = { 0.25045537, 0.48303426, 0.98555979,
                                  0.51948512, 0.61289453, 0.12062867,
                                  0.8263408 , 0.60306013, 0.54506801 };

    floatVector answer = { 0.55339061, -0.59325289,  0.92984685,
                          -0.83130342, -0.25274097, -1.5877536 ,
                           1.67911302, -0.83554021,  3.47033811 };

    floatVector result;
    errorOut error = constitutiveTools::pullBackAlmansiStrain( almansiStrain, deformationGradient, result );

    if ( error ){
        error->print();
        results << "testPullBackAlmansiStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals( answer, result )  );

    //Test the jacobians
    floatVector resultJ;
    floatMatrix dEde, dEdF;

    error = constitutiveTools::pullBackAlmansiStrain( almansiStrain, deformationGradient, resultJ, dEde, dEdF );

    if ( error ){
        error->print();
        results << "testPullBackAlmansiStrain & False\n";
        return 1;
    }

    BOOST_CHECK( vectorTools::fuzzyEquals( answer, resultJ )  );

    //Testing dEde    
    floatType eps = 1e-6;
    for ( unsigned int i = 0; i < almansiStrain.size(); i++ ){
        floatVector delta( almansiStrain.size(), 0 );
        delta[i] = eps * fabs( almansiStrain[i] ) + eps;

        error = constitutiveTools::pullBackAlmansiStrain( almansiStrain + delta, deformationGradient, resultJ );

        if ( error ){
            error->print();
            results << "testPullBackAlmansiStrain & False\n";
            return 1;
        }

        floatVector gradCol = ( resultJ - result ) / delta[i];

        for ( unsigned int j = 0; j < gradCol.size(); j++ ){
            BOOST_CHECK( vectorTools::fuzzyEquals( gradCol[j], dEde[j][i] )  );
        }
    }

    //Testing dEdF
    for ( unsigned int i = 0; i < deformationGradient.size(); i++ ){
        floatVector delta( deformationGradient.size(), 0 );
        delta[i] = eps * fabs( deformationGradient[i] ) + eps;

        error = constitutiveTools::pullBackAlmansiStrain( almansiStrain, deformationGradient + delta, resultJ );

        if ( error ){
            error->print();
            results << "testPullBackAlmansiStrain & False\n";
            return 1;
        }

        floatVector gradCol = ( resultJ - result ) / delta[i];

        for ( unsigned int j = 0; j < gradCol.size(); j++ ){
            BOOST_CHECK( vectorTools::fuzzyEquals( gradCol[j], dEdF[j][i] )  );
        }
    }

    results << "testPullBackAlansiStrain & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeRightCauchyGreen ){
    /*!
     * Test the computation of the Right Cauchy-Green deformation tensor
     * \param &results: The output file
     */

    floatVector deformationGradient = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    
    floatVector answer = { 66, 78, 90, 78, 93, 108, 90, 108, 126 };

    floatVector result;

    errorOut error = constitutiveTools::computeRightCauchyGreen( deformationGradient, result );

    if ( error ){
        error->print();
        results << "testComputeRightCauchyGreen & False\n";
        return 1;
    }

    if( !vectorTools::fuzzyEquals( result, answer ) ){
        results << "testComputeRightCauchyGreen (test 1) & False\n";
        return 1;
    }

    //Test Jacobian

    floatVector resultJ;
    floatMatrix dCdF;

    error = constitutiveTools::computeRightCauchyGreen( deformationGradient, resultJ, dCdF );
    
    if ( error ){
        error->print( );
        results << "testComputeRightCauchyGreen & False\n";
    }

    BOOST_CHECK( vectorTools::fuzzyEquals( resultJ, answer )  );

    floatType eps = 1e-6;
    for ( unsigned int i = 0; i < deformationGradient.size( ); i++ ){
        floatVector delta( deformationGradient.size( ), 0 );
        delta[ i ] = eps * fabs( deformationGradient[ i ] ) + eps;

        error = constitutiveTools::computeRightCauchyGreen( deformationGradient + delta, resultJ );

        if ( error ){
            error->print( );
            results << "testComputeRightCauchyGreen & False\n";
        }

        floatVector gradCol = ( resultJ - result ) / delta[ i ];

        for ( unsigned int j = 0; j < gradCol.size( ); j++ ){
            BOOST_CHECK( vectorTools::fuzzyEquals( gradCol[ j ], dCdF[ j ][ i ] )  );
        }
    }

    results << "testComputeRightCauchyGreen & True\n";
    return 0;
}

BOOST_AUTO_TEST_CASE( testComputeSymmetricPart ){
    /*!
     * Test the computation of the symmetric part of a matrix
     *
     * \param &results: The output file
     */

    floatVector A = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    
    floatVector answer = { 1., 3., 5., 3., 5., 7., 5., 7., 9. };
    
    floatVector result;
    
    errorOut error = constitutiveTools::computeSymmetricPart( A, result );
    
    if ( error ){
        error->print( );
        results << "testComputeSymmetricPart & False\n";
        return 1;
    }
    
    if( !vectorTools::fuzzyEquals( result, answer ) ){
        results << "testComputeSymmetricPart (test 1) & False\n";
        return 1;
    }
    
    floatVector resultJ;
    floatMatrix dSymmAdA;
    
    error = constitutiveTools::computeSymmetricPart( A, resultJ, dSymmAdA );
    
    if ( error ){
        error->print( );
        results << "testComputeSymmetricPart & False\n";
        return 1;
    }
    
    BOOST_CHECK( vectorTools::fuzzyEquals( resultJ, answer )  );
    
    floatType eps = 1e-6;
    for ( unsigned int i = 0; i < A.size( ); i++ ){
        floatVector delta( A.size( ), 0 );
        delta[ i ] = eps * fabs( A[ i ] ) + eps;
    
        error = constitutiveTools::computeSymmetricPart( A + delta, resultJ );
    
        if ( error ){
            error->print( );
            results << "testCoputeSymmetricPart & False\n";
            return 1;
        }
    
        floatVector gradCol = ( resultJ - result ) / delta[ i ];
    
        for ( unsigned int j = 0; j < gradCol.size( ); j++ ){
            BOOST_CHECK( vectorTools::fuzzyEquals( gradCol[ j ], dSymmAdA[ j ][ i ] )  );
        }
    }

    results << "testComputeSymmetricPart & True\n";
    return 0;
}
