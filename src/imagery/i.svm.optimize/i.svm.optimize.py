#!/usr/bin/env python3

############################################################################
#
# MODULE:      i.svm.optimize
#
# AUTHOR(S):   Māris Nartišs
#
# PURPOSE:     Finds optimal hyperparameter settings for SVM classifier
#
# COPYRIGHT:   (C) 2024 by Māris Nartišs, and the GRASS development team
#
#              This program is free software under the GNU General Public
#              License (>=v2). Read the file COPYING that comes with GRASS
#              for details.
#
#############################################################################

# %Module
# % description: Finds optimal SVM classifier hyperparameters
# % keyword: imagery
# % keyword: classification
# %end
#
# %option G_OPT_I_GROUP
# % description: Maps with feature values (attributes)
# % required : yes
# %end
#
# %option G_OPT_R_MAP
# % key: trainingmap
# % description: Map with training labels or target values
# % required : yes
# %end
#
# %option G_OPT_R_MAP
# % key: validationmap
# % description: Map with validation labels or target values
# % required : yes
# %end
#
# %option G_OPT_F_OUTPUT
# % description: Name of hyperparameter search log file (- for standard output)
# % required : no
# %end

import sys
import os

from grass.exceptions import GrassError
import grass.script as gs
from grass.pygrass.modules import Module

gs.utils.set_path(modulename="i.svm.optimize", dirname="etc")

def main():
    print(options)


if __name__ == "__main__":
    options, flags = gs.parser()

    try:
        from pymoo.core.problem import ElementwiseProblem
        from pymoo.core.problem import StarmapParallelization
        from pymoo.optimize import minimize
        from pymoo.algorithms.soo.nonconvex.pso import PSO
        from problems import SVMProblem
    except Exception as e:
        gs.warning(e)
        gs.fatal(_("Cannot import pymoo. Please intall the library first."))

    try:
        i_svm_train = Module("i.svm.ttrain")
    except GrassError:
        gs.fatal(_("Can not run i.svm.train. Is GRASS compiled with SVM support?"))

    sys.exit(main())
