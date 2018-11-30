#ifndef _COMPADRE_LINEAR_ALGEBRA_DEFINITIONS_HPP_
#define _COMPADRE_LINEAR_ALGEBRA_DEFINITIONS_HPP_

#include "Compadre_LinearAlgebra_Declarations.hpp"

namespace Compadre {
namespace GMLS_LinearAlgebra {

KOKKOS_INLINE_FUNCTION
void createM(const member_type& teamMember, scratch_matrix_type M_data, scratch_matrix_type weighted_P, const int columns, const int rows) {
	/*
	 * Creates M = P^T * W * P
	 */

	const int target_index = teamMember.league_rank();
	double * p_data = weighted_P.data();

	for (int i=0; i<columns; ++i) {
		// offdiagonal entries
		for (int j=0; j<i; ++j) {
			double M_data_entry_i_j = 0;
			teamMember.team_barrier();

			Kokkos::parallel_reduce(Kokkos::TeamThreadRange(teamMember,rows), [=] (const int k, double &entry_val) {
				// assumes layout left input matrix
				double val_i = *(p_data + i*weighted_P.dimension_0() + k);
				double val_j = *(p_data + j*weighted_P.dimension_0() + k);
				entry_val += val_i*val_j;
			}, M_data_entry_i_j );

			Kokkos::single(Kokkos::PerTeam(teamMember), [&] () {
				M_data(i,j) = M_data_entry_i_j;
				M_data(j,i) = M_data_entry_i_j;
			});
			teamMember.team_barrier();
		}
		// diagonal entries
		double M_data_entry_i_j = 0;
		teamMember.team_barrier();

		Kokkos::parallel_reduce(Kokkos::TeamThreadRange(teamMember,rows), [=] (const int k, double &entry_val) {
			// assumes layout left input matrix
			double val = *(p_data + i*weighted_P.dimension_0() + k);
			entry_val += val*val;
		}, M_data_entry_i_j );

		Kokkos::single(Kokkos::PerTeam(teamMember), [&] () {
			M_data(i,i) = M_data_entry_i_j;
		});
		teamMember.team_barrier();
	}
	teamMember.team_barrier();

//	for (int i=0; i<columns; ++i) {
//		for (int j=0; j<columns; ++j) {
//			std::cout << "(" << i << "," << j << "):" << M_data(i,j) << std::endl;
//		}
//	}
}


KOKKOS_INLINE_FUNCTION
void largestTwoEigenvectorsThreeByThreeSymmetric(const member_type& teamMember, scratch_matrix_type V, scratch_matrix_type PtP, const int dimensions) {

	Kokkos::single(Kokkos::PerTeam(teamMember), [&] () {
		// put in a power method here and a deflation by first found eigenvalue
		double eigenvalue_relative_tolerance = 1e-6; // TODO: use something smaller, but really anything close is acceptable for this manifold
		double v[3] = {1, 1, 1};
		double v_old[3] = {1, 1, 1};

		double error = 1;
		double norm_v;

		while (error > eigenvalue_relative_tolerance) {

			double tmp1 = v[0];
			v[0] = PtP(0,0)*tmp1 + PtP(0,1)*v[1];
			if (dimensions>2) v[0] += PtP(0,2)*v[2];

			double tmp2 = v[1];
			v[1] = PtP(1,0)*tmp1 + PtP(1,1)*tmp2;
			if (dimensions>2) v[1] += PtP(1,2)*v[2];

			if (dimensions>2)
				v[2] = PtP(2,0)*tmp1 + PtP(2,1)*tmp2 + PtP(2,2)*v[2];

			norm_v = v[0]*v[0] + v[1]*v[1];
			if (dimensions>2) norm_v += v[2]*v[2];
			norm_v = std::sqrt(norm_v);

			v[0] = v[0] / norm_v;
			v[1] = v[1] / norm_v;
			if (dimensions>2) v[2] = v[2] / norm_v;

			error = (v[0]-v_old[0])*(v[0]-v_old[0]) + (v[1]-v_old[1])*(v[1]-v_old[1]);
			if (dimensions>2) error += (v[2]-v_old[2])*(v[2]-v_old[2]);
			error = std::sqrt(error);
			error /= norm_v;


			v_old[0] = v[0];
			v_old[1] = v[1];
			if (dimensions>2) v_old[2] = v[2];
		}

		double dot_product;
		double norm;

		// if 2D, orthonormalize second vector
		if (dimensions==2) {

			for (int i=0; i<2; ++i) {
				V(0,i) = v[i];
			}

			// orthonormalize second eigenvector against first
			V(1,0) = 1.0; V(1,1) = 1.0;
			dot_product = V(0,0)*V(1,0) + V(0,1)*V(1,1);
			V(1,0) -= dot_product*V(0,0);
			V(1,1) -= dot_product*V(0,1);

			norm = std::sqrt(V(1,0)*V(1,0) + V(1,1)*V(1,1));
			V(1,0) /= norm;
			V(1,1) /= norm;

		} else { // otherwise, work on second eigenvalue

			for (int i=0; i<3; ++i) {
				V(0,i) = v[i];
				for (int j=0; j<3; ++j) {
					PtP(i,j) -= norm_v*v[i]*v[j];
				}
			}

			error = 1;
			v[0] = 1; v[1] = 1; v[2] = 1;
			v_old[0] = 1; v_old[1] = 1; v_old[2] = 1;
			while (error > eigenvalue_relative_tolerance) {

				double tmp1 = v[0];
				v[0] = PtP(0,0)*tmp1 + PtP(0,1)*v[1] + PtP(0,2)*v[2];

				double tmp2 = v[1];
				v[1] = PtP(1,0)*tmp1 + PtP(1,1)*tmp2 + PtP(1,2)*v[2];

				v[2] = PtP(2,0)*tmp1 + PtP(2,1)*tmp2 + PtP(2,2)*v[2];

				norm_v = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

				v[0] = v[0] / norm_v;
				v[1] = v[1] / norm_v;
				v[2] = v[2] / norm_v;

				error = std::sqrt((v[0]-v_old[0])*(v[0]-v_old[0]) + (v[1]-v_old[1])*(v[1]-v_old[1]) + (v[2]-v_old[2])*(v[2]-v_old[2])) / norm_v;

				v_old[0] = v[0];
				v_old[1] = v[1];
				v_old[2] = v[2];
			}

			for (int i=0; i<3; ++i) {
				V(1,i) = v[i];
			}

			// orthonormalize second eigenvector against first
			dot_product = V(0,0)*V(1,0) + V(0,1)*V(1,1) + V(0,2)*V(1,2);

			V(1,0) -= dot_product*V(0,0);
			V(1,1) -= dot_product*V(0,1);
			V(1,2) -= dot_product*V(0,2);

			norm = std::sqrt(V(1,0)*V(1,0) + V(1,1)*V(1,1) + V(1,2)*V(1,2));
			V(1,0) /= norm;
			V(1,1) /= norm;
			V(1,2) /= norm;

			// orthonormalize third eigenvector against first and second
			V(2,0) = 1.0; V(2,1) = 1.0; V(2,2) = 1.0;
			dot_product = V(0,0)*V(2,0) + V(0,1)*V(2,1) + V(0,2)*V(2,2);
			V(2,0) -= dot_product*V(0,0);
			V(2,1) -= dot_product*V(0,1);
			V(2,2) -= dot_product*V(0,2);

			norm = std::sqrt(V(2,0)*V(2,0) + V(2,1)*V(2,1) + V(2,2)*V(2,2));
			V(2,0) /= norm;
			V(2,1) /= norm;
			V(2,2) /= norm;

			dot_product = V(1,0)*V(2,0) + V(1,1)*V(2,1) + V(1,2)*V(2,2);
			V(2,0) -= dot_product*V(1,0);
			V(2,1) -= dot_product*V(1,1);
			V(2,2) -= dot_product*V(1,2);

			norm = std::sqrt(V(2,0)*V(2,0) + V(2,1)*V(2,1) + V(2,2)*V(2,2));
			V(2,0) /= norm;
			V(2,1) /= norm;
			V(2,2) /= norm;

		}

	});
}

}; // GMLS_LinearAlgebra
}; // Compadre

#endif
