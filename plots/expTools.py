import os
from itertools import *
import subprocess




def nbTiles(comb):
    # Si -nbtiles est spécifié directement
    if "-nbtiles" in comb:
        return comb["-nbtiles"]**2

    # Récupérer la taille de l'image (défaut: 1024)
    dim = comb.get("-s", 1024)

    # Si -ts (tile-size) est spécifié : tuiles carrées
    if "-ts" in comb:
        tile_size = comb["-ts"]
        nb_tiles_x = dim // tile_size
        nb_tiles_y = dim // tile_size
        return nb_tiles_x * nb_tiles_y

    # Sinon, utiliser -tw et -th pour des tuiles rectangulaires
    tile_w = comb.get("-tw", comb.get("-th", 32))  # Largeur de la tuile
    tile_h = comb.get("-th", 32)                   # Hauteur de la tuile

    nb_tiles_x = dim // tile_w
    nb_tiles_y = dim // tile_h

    return nb_tiles_x * nb_tiles_y


def iterateur_option(dicopt, sep =' '):
    options = []

    for opt, listval in dicopt.items():
        optlist = []
        for val in listval:
            optlist += [(opt, val)]
        options += [optlist]

    for combo in product(*options):
        # Créer le dictionnaire des options
        opt_dict = {opt: val for opt, val in combo}
        # Créer la chaîne de commande
        opt_str = ' '.join(opt + sep + str(val) for opt, val in combo)
        yield opt_dict, opt_str


def execute(commande, option, nbruns, verbose=True, filter=lambda x: True, easyPath='.'):
    path = os.getcwd()
    os.chdir(easyPath)
    for i in range(nbruns):
        for opt_dict, opt_str in iterateur_option(option):
            if (not filter(opt_dict)):
                if (verbose):
                    print("Filtered out : " + commande + " " + opt_str)
                continue
            if (verbose):
                print(commande + " " + opt_str)
            if(subprocess.call([commande + " " + opt_str], shell=True) == 1):
                os.chdir(path)
                return ("Error on the command used")
    if (verbose):
        print("Experiences done")
    os.chdir(path)

def execute_simple(cmd):
    subprocess.run(cmd, shell=True, check=True)
