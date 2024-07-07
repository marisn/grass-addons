import json

from pymoo.core.problem import ElementwiseProblem

import grass.script as grass
from grass.exceptions import CalledModuleError
from grass.script.core import tempname


class SVMProblem(ElementwiseProblem):
    def __init__(self, conf, q, **kwargs):
        super().__init__(
            n_var=conf["n_var"], n_obj=1, xl=conf["xl"], xu=conf["xu"], **kwargs
        )
        self.conf = conf
        self.q = q

    def _evaluate(self, x, out, *args, **kwargs):
        postfix = gs.core.tempname(10)
        try:
            grass.run_command(
                "i.svm.train",
                group=self.conf["group_t"],
                subgroup=self.conf["subgroup_t"],
                trainingmap=self.conf["training"],
                signaturefile=self.conf["signature"] + postfix,
                cost=(
                    x[self.conf["variables"]["cost"]["index"]]
                    if self.conf["variables"]["cost"]["in_use"]
                    else self.conf["variables"]["cost"]["value"]
                ),
                gamma=(
                    x[self.conf["variables"]["gamma"]["index"]]
                    if self.conf["variables"]["gamma"]["in_use"]
                    else self.conf["variables"]["gamma"]["value"]
                ),
                degree=(
                    x[self.conf["variables"]["degree"]["index"]]
                    if self.conf["variables"]["degree"]["in_use"]
                    else self.conf["variables"]["degree"]["value"]
                ),
                overwrite=True,
                quiet=True,
            )
            grass.run_command(
                "i.svm.predict",
                group=self.config["group_v"],
                subgroup=self.config["subgroup_v"],
                signaturefile=self.config["signature"] + postfix,
                output=self.config["output"] + postfix,
                overwrite=True,
                quiet=True,
            )
            evaluation = grass.read_command(
                "r.kappa",
                reference=self.config["validation"],
                classification=self.config["output"] + postfix,
                format="json",
                quiet=True,
            )
        except CalledModuleError:
            print(f"\n===== FAILURE WITH C:{x[0]} gamma:{x[1]}\n")
            out["F"] = 10
            self.q.put(f"{x[0]},{x[1]},NA,NA,NA")
            return
        try:
            kappa = json.loads(evaluation)
            mcc = float(kappa["mcc"])
        except (json.decoder.JSONDecodeError, KeyError, ValueError, TypeError):
            self.q.put(f"{x[0]},{x[1]},NA,NA,NA")
            out["F"] = 10
            return
        if mcc < -1:
            out["F"] = 10
        else:
            out["F"] = 1 - mcc
        self.q.put(
            f"{x[0]},{x[1]},{kappa['mcc']},{kappa['kappa']},{kappa['overall_accuracy']}"
        )
        try:
            grass.read_command(
                "g.remove",
                flags="f",
                type="raster",
                name=self.config["output"] + postfix,
                quiet=True,
            )
        except CalledModuleError:
            print("err removing raster")
