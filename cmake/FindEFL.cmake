INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES (EDJE REQUIRED edje>=0.9.9.063)
PKG_CHECK_MODULES (ECORE REQUIRED ecore>=0.9.9.063)
PKG_CHECK_MODULES (ECORE_EVAS REQUIRED ecore-evas>=0.9.9.063)
PKG_CHECK_MODULES (EFLDEPS REQUIRED
  eina-0>=0.9.9.063
  evas>=0.9.9.063
  ecore>=0.9.9.063
  ecore-file>=0.9.9.063
  ecore-evas>=0.9.9.063
  edje>=0.9.9.063)
PKG_CHECK_MODULES (EINA REQUIRED eina-0>=0.9.9.063)
PKG_CHECK_MODULES (ECORE_X REQUIRED ecore-x>=0.9.9.063)
PKG_CHECK_MODULES (EVAS REQUIRED evas>=0.9.9.063)

