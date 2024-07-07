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
# % required: yes
# % guisection: inputs
# %end
#
# %option G_OPT_R_MAP
# % key: trainingmap
# % description: Map with training labels or target values
# % required: yes
# % guisection: inputs
# %end
#
# %option G_OPT_R_MAP
# % key: validationmap
# % description: Map with validation labels or target values
# % required: yes
# % guisection: inputs
# %end
#
# %option G_OPT_F_OUTPUT
# % description: Name of hyperparameter search log file (- for standard output)
# % required: no
# % guisection: outputs
# %end
#
# %option
# % key: measure
# % description: Measure to optimize
# % required: no
# % type: string
# % options: mcc,acc,kappa
# % answer: mcc
# % guisection: optimization
# %end
#
# %option
# % key: population
# % description: Population size
# % required: no
# % type: integer
# % options: 0-
# % answer: 100
# % guisection: optimization
# %end
#
# %option
# % key: steps
# % description: How many times to execute optimization
# % required: no
# % type: integer
# % options: 0-
# % answer: 50
# % guisection: optimization
# %end
#
# %option
# % key: seed
# % description: Random seed for paricle initial position initialization
# % required: no
# % type: integer
# % options: 0-
# % answer: 42
# % guisection: optimization
# %end
#
# %option
# % key: type
# % description: SVM type
# % required: no
# % type: string
# % options: c_svc,nu_svc,one_class,epsilon_svr,nu_svr
# % answer: c_svc
# % guisection: svm
# %end
#
# %option
# % key: kernel
# % description: SVM kernel type
# % required: no
# % type: string
# % options: linear,poly,rbf,sigmoid
# % answer: rbf
# % guisection: svm
# %end
#
# %option
# % key: gamma
# % description: Gamma in kernel function
# % required: no
# % type: double
# % multiple: yes
# % options: 0-
# % answer: 2
# % guisection: variables
# %end
#
# %option
# % key: cost
# % description: Cost of constraints violation
# % required: no
# % type: double
# % multiple: yes
# % options: 0-
# % answer: 100
# % guisection: variables
# %end
#
# %option
# % key: degree
# % description: Degree in kernel function
# % required: no
# % type: integer
# % multiple: yes
# % options: 0-
# % answer: 3
# % guisection: variables
# %end
#
# %option
# % key: coef0
# % description: coef0 in kernel function
# % required: no
# % type: double
# % multiple: yes
# % options: 0-
# % answer: 0
# % guisection: variables
# %end
#
# %option
# % key: eps
# % description: Tolerance of termination criterion
# % required: no
# % type: double
# % multiple: yes
# % options: 0-
# % answer: 0.001
# % guisection: variables
# %end
#
# %option
# % key: nu
# % description: The parameter nu of nu-SVC, one-class SVM, and nu-SVR
# % required: no
# % type: double
# % multiple: yes
# % options: 0-
# % answer: 0.5
# % guisection: variables
# %end
#

import sys
import os

from grass.exceptions import GrassError
import grass.script as gs
from grass.pygrass.modules import Module

gs.utils.set_path(modulename="i.svm.optimize", dirname="etc")


def determine_variable(key, var_count, conf):
    val_list = options[key].split(",")
    val_count = len(val_list)

    if val_count < 1 or val_count > 2:
        gs.fatal(f"Provide one or two values for the {key} parameter")

    var = []
    for val in val_list:
        if conf["variables"][key]["is_int"]:
            var.append(int(val))
        else:
            var.append(float(val))

    if len(var) == 2:
        var.sort()
        conf["xl"].append(var[0])
        conf["xu"].append(var[1])
        conf["variables"][key]["in_use"] = True
        conf["variables"][key]["index"] = var_count
        return True

    conf["variables"][key]["value"] = var[0]
    return False


def main():
    var_count = 0
    conf = {
        "variables": {
            "gamma": {"in_use": False, "is_int": False, "index": None, "value": None},
            "cost": {"in_use": False, "is_int": False, "index": None, "value": None},
            "degree": {"in_use": False, "is_int": True, "index": None, "value": None},
        },
        "xl": [],
        "xu": [],
    }

    # Determine variables to optimize
    for key in conf["variables"].keys():
        if determine_variable(key, var_count, conf):
            var_count += 1
    if var_count == 0:
        gs.fatal(_("At least one variable must have its lower and upper range set"))

    # Create a problem

    # Execute optimization


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
        i_svm_train = Module("i.svm.train")
    except GrassError:
        gs.fatal(_("Can not run i.svm.train. Is GRASS compiled with SVM support?"))

    sys.exit(main())
