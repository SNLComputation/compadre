#ifndef _GMLS_PYTHON_HPP_
#define _GMLS_PYTHON_HPP_

#include <GMLS.hpp>
#include <Kokkos_Core.hpp>
#include <Kokkos_Random.hpp>
#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
//#include <assert.h>

void initializeKokkos() {
	Kokkos::initialize();

}

void finalizeKokkos() {
	Kokkos::finalize(); 
}

class GMLS_Python {

private:

	GMLS* gmls_object;

public:
	GMLS_Python(const int poly_order, std::string dense_solver_type, const int manifold_poly_order, const int dimensions) {
		gmls_object = new GMLS(poly_order, dense_solver_type, manifold_poly_order, dimensions);
		// initialized, but values not set
	}

	~GMLS_Python() { delete gmls_object; }

	void setWeightingOrder(int regular_weight, int manifold_weight = -1) {
		if (manifold_weight < 0) manifold_weight = regular_weight;
		gmls_object->setManifoldWeightingPower(manifold_weight);
		gmls_object->setWeightingPower(regular_weight);
	}

	void setNeighbors(PyObject* pyObjectArray_in) {
        // cast as a numpy array
        PyArrayObject *np_arr_in = reinterpret_cast<PyArrayObject*>(pyObjectArray_in);

        // copy data into Kokkos View
        // read in size in each dimension
        npy_intp* dims_in = PyArray_DIMS(np_arr_in);
        int npyLength1D = dims_in[0];
        int npyLength2D = dims_in[1];

        // create Kokkos View on host to copy into
    	Kokkos::View<int**, Kokkos::HostSpace>	neighbor_lists("neighbor lists", npyLength1D, npyLength2D); // first column is # of neighbors

    	// overwrite existing data assuming a 2D layout
        for (int i = 0; i < npyLength1D; ++i)
        {
                for (int j = 0; j < npyLength2D; ++j)
                {
                        int* val = (int*)PyArray_GETPTR2(np_arr_in, i, j);
                        neighbor_lists(i,j) = *val;
                }
        }

        // set values from Kokkos View
        gmls_object->setNeighborLists(neighbor_lists);
	}

	void setSourceSites(PyObject* pyObjectArray_in) {
        // cast as a numpy array
        PyArrayObject *np_arr_in = reinterpret_cast<PyArrayObject*>(pyObjectArray_in);

        // copy data into Kokkos View
        // read in size in each dimension
        npy_intp* dims_in = PyArray_DIMS(np_arr_in);
        int npyLength1D = dims_in[0];
        int npyLength2D = dims_in[1];

//        assert(npyLength2Dd == gmls_object->getDimensions());

        // create Kokkos View on host to copy into
    	Kokkos::View<double**, Kokkos::HostSpace> source_coords("neighbor coordinates", npyLength1D, npyLength2D);

        // overwrite existing data assuming a 2D layout
        for (int i = 0; i < npyLength1D; ++i)
        {
                for (int j = 0; j < npyLength2D; ++j)
                {
                        double* val = (double*)PyArray_GETPTR2(np_arr_in, i, j);
                        source_coords(i,j) = *val;
                }
        }

        // set values from Kokkos View
        gmls_object->setSourceSites(source_coords);
	}

	void setTargetSites(PyObject* pyObjectArray_in) {
        // cast as a numpy array
        PyArrayObject *np_arr_in = reinterpret_cast<PyArrayObject*>(pyObjectArray_in);

        // copy data into Kokkos View
        // read in size in each dimension
        npy_intp* dims_in = PyArray_DIMS(np_arr_in);
        int npyLength1D = dims_in[0];
        int npyLength2D = dims_in[1];

        // create Kokkos View on host to copy into
    	Kokkos::View<double**, Kokkos::HostSpace> target_coords("neighbor coordinates", npyLength1D, npyLength2D);

//    	assert(npyLength2Dd == gmls_object->getDimensions());

        // overwrite existing data assuming a 2D layout
        for (int i = 0; i < npyLength1D; ++i)
        {
                for (int j = 0; j < npyLength2D; ++j)
                {
                        double* val = (double*)PyArray_GETPTR2(np_arr_in, i, j);
                        target_coords(i,j) = *val;
                }
        }

        // set values from Kokkos View
        gmls_object->setTargetSites(target_coords);
	}

	void setWindowSizes(PyObject* pyObjectArray_in) {
        // cast as a numpy array
        PyArrayObject *np_arr_in = reinterpret_cast<PyArrayObject*>(pyObjectArray_in);

        // copy data into Kokkos View
        // read in size in each dimension
        npy_intp* dims_in = PyArray_DIMS(np_arr_in);
        int npyLength1D = dims_in[0];

        // create Kokkos View on host to copy into
    	Kokkos::View<double*, Kokkos::HostSpace> epsilon("h supports", npyLength1D);

        // overwrite existing data assuming a 2D layout
        for (int i = 0; i < npyLength1D; ++i)
        {
//                for (int j = 0; j < npyLength2D; ++j)
//                {
                        double* val = (double*)PyArray_GETPTR1(np_arr_in, i);
                        epsilon(i) = *val;
//                }
        }

        // set values from Kokkos View
        gmls_object->setWindowSizes(epsilon);
	}

	void generatePointEvaluationStencil() {
		gmls_object->addTargets(ReconstructionOperator::TargetOperation::ScalarPointEvaluation, gmls_object->getDimensions());
		gmls_object->generateAlphas();
	}


	double applyStencilTo0Tensor(int target_num, PyObject* pyObjectArray_neighborList, PyObject* pyObjectArray_sourceData) {
        	// cast as a numpy array
        	PyArrayObject *np_arr_neighborlist = reinterpret_cast<PyArrayObject*>(pyObjectArray_neighborList);
        	PyArrayObject *np_arr_sourcedata = reinterpret_cast<PyArrayObject*>(pyObjectArray_sourceData);

        	// copy data into Kokkos View
        	// read in size in each dimension
        	npy_intp* dims_in = PyArray_DIMS(np_arr_sourcedata);

        	int* loop_size = (int*)PyArray_GETPTR2(np_arr_neighborlist, target_num, 0);

		double target_evaluation;
        	Kokkos::parallel_reduce(Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0,*loop_size), [=](int i, double &temp_target_evaluation) {
        		int* neighbor_id = (int*)PyArray_GETPTR2(np_arr_neighborlist, target_num, i+1); // first index is size in neighborlist
		double *source_val = (double*)PyArray_GETPTR1(np_arr_sourcedata, *neighbor_id);
		temp_target_evaluation += (*source_val)*gmls_object->getAlpha0TensorTo0Tensor(ReconstructionOperator::TargetOperation::ScalarPointEvaluation, target_num, i);
		}, target_evaluation);

        	return target_evaluation;
	}

	PyObject* applyStencilTo0Tensor(PyObject* pyObjectArray_neighborList, PyObject* pyObjectArray_sourceData) {
        	// cast as a numpy array
        	PyArrayObject *np_arr_neighborlist = reinterpret_cast<PyArrayObject*>(pyObjectArray_neighborList);
        	PyArrayObject *np_arr_sourcedata = reinterpret_cast<PyArrayObject*>(pyObjectArray_sourceData);

        	// copy data into Kokkos View
        	// read in size in each dimension
        	npy_intp* dims_in = PyArray_DIMS(np_arr_sourcedata);

        	npy_intp* dims_nl = PyArray_DIMS(np_arr_neighborlist);
                // set dimensions
                npy_intp dims_out[1] = {dims_nl[0]};

                // allocate memory for array 
                PyObject *pyObjectArray_out = PyArray_SimpleNew(1, dims_out, NPY_DOUBLE);
                if (!pyObjectArray_out) {
                        printf("Out of memory.\n");
                }

                // recast as a numpy array and write assuming a 1D layout
                PyArrayObject *np_arr_out = reinterpret_cast<PyArrayObject*>(pyObjectArray_out);
        	Kokkos::parallel_for(Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0,dims_out[0]), [=](int i) {
			// compute each entry
        		int* loop_size = (int*)PyArray_GETPTR2(np_arr_neighborlist, i, 0);

			double target_evaluation = 0;
			for (int j=0, N=*loop_size; j<N; ++j) {
        			int* neighbor_id = (int*)PyArray_GETPTR2(np_arr_neighborlist, i, j+1); // first index is size in neighborlist
				double *source_val = (double*)PyArray_GETPTR1(np_arr_sourcedata, *neighbor_id);
				target_evaluation += (*source_val)*gmls_object->getAlpha0TensorTo0Tensor(ReconstructionOperator::TargetOperation::ScalarPointEvaluation, i, j);
			}

			double* val = (double*)PyArray_GETPTR1(np_arr_out, i);
			*val = target_evaluation;
                });

                // return the Python object
                return pyObjectArray_out;
	}

	int getNP(const int poly_order, const int dimensions) {
		// number of points needed for unisolvency
		return GMLS::getNP(poly_order, dimensions);
        }
};

#endif


