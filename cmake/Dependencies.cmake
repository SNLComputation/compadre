if (Compadre_USE_MKL)
tribits_package_define_dependencies(
  LIB_REQUIRED_PACKAGES
    KokkosCore KokkosContainers KokkosAlgorithms
  LIB_REQUIRED_TPLS
    MKL
  LIB_OPTIONAL_TPLS
    MPI
  )
elseif (Compadre_USE_CUDA)
tribits_package_define_dependencies(
  LIB_REQUIRED_PACKAGES
    KokkosCore KokkosContainers KokkosAlgorithms
  LIB_REQUIRED_TPLS
    CUSOLVER CUBLAS
  LIB_OPTIONAL_TPLS
    MPI
  )
else() # Compadre_USE_LAPACK
tribits_package_define_dependencies(
  LIB_REQUIRED_PACKAGES
    KokkosCore KokkosContainers KokkosAlgorithms
  LIB_REQUIRED_TPLS
    LAPACK BLAS
  LIB_OPTIONAL_TPLS
    MPI
  )
endif()
