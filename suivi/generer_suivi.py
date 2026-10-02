#!/usr/bin/env python3
"""Genere suivi/SUIVI_ELIOTT.pdf : plan de travail d'Eliott et avancement.

Aucune dependance : le PDF est ecrit a la main (polices standard Helvetica).
Pour mettre a jour : modifier MAJ, PROCHAINE et les statuts dans SECTIONS,
puis lancer  python3 suivi/generer_suivi.py
"""
import os
import textwrap

MAJ = "2 octobre 2026"

# Statuts : "fait", "encours", "afaire", "bloque"
PROCHAINE = [
    "Corriger le destructeur (Environnement::~Environnement() = default;), puis",
    "ecrire le constructeur : remplir Etat a partir des JSON carte et armoire.",
]

SECTIONS = [
    ("1. Roue cyclique (roue.cpp)", [
        ("fait", "roue_avancer : bouclage dans [0, n-1], pas negatifs geres"),
        ("fait", "roue_distance : plus court cote, dans [0, n/2]"),
        ("fait", "Doc des deux fonctions + #include <algorithm>"),
        ("fait", "Commit"),
    ]),
    ("2. Environnement / simulateur (environnement.cpp)", [
        ("fait", "Ajouter environnement.o a ROBOT dans le Makefile"),
        ("fait", "struct Environnement::Etat : grille, robot, selecteur, armoire 2D "
                 "(optional), positions fixes, residents, objet porte, resident vise, departs"),
        ("encours", "Destructeur ~Environnement() = default (apres la struct)"),
        ("encours", "Constructeur : lire carte + armoire (paire_entiers, casiers -> grille 2D)"),
        ("afaire", "connaissances() : tout SAUF la grille et les objets"),
        ("afaire", "percevoir() : 4 voisins N/S/E/O, devant_armoire, contenu_casier"),
        ("afaire", "viser_resident(), robot(), selecteur()"),
        ("bloque", "executer() : legalite des 7 actions (section 3.4), CHERCHER avec "
                   "bouclage des colonnes (annexe C) -> besoin du texte de l'enonce"),
    ]),
    ("3. Validation des fichiers (section 5.6, module a part)", [
        ("afaire", "Fichier absent / illisible / pas UTF-8, JSON invalide"),
        ("afaire", "Champ obligatoire absent ou mauvais type (4 fichiers)"),
        ("afaire", "Grille non rectangulaire, dimensions incoherentes, position hors grille"),
        ("afaire", "Symboles P / A / D / R coherents avec les positions"),
        ("afaire", "Residents en double, scenario d'une autre carte, resident absent"),
        ("afaire", "Casiers : hors armoire, doublon, etiquette incoherente, casier_depart hors armoire"),
        ("afaire", "Dictionnaire : emotion/intensite inconnue, mot en double"),
        ("afaire", "Erreur -> message stderr, code retour non nul, AUCUNE trace ecrite"),
    ]),
    ("4. Affichage texte (debogage + video)", [
        ("afaire", "Grille redessinee a chaque pas : robot + murs deja decouverts"),
    ]),
    ("5. Tests", [
        ("afaire", "Test 4 : selecteur de l'armoire, bouclage colonne 7 <-> 0"),
        ("afaire", "Test 6 : un fichier casse par categorie de 5.6"),
        ("afaire", "Test 7 : resident mure sur une petite grille (avec Axel)"),
    ]),
    ("6. Ensemble avec Axel (fin de projet)", [
        ("afaire", "main.cpp : boucle Environnement <-> Agent, trace, resume"),
        ("afaire", "Relecture croisee : agent.cpp, graphe.cpp, carte_robot.cpp"),
        ("afaire", "Rapport 3 pages (architecture, replanification, repli, 3 echecs)"),
        ("afaire", "Video 1 min 30 (robot qui se cogne et replanifie)"),
        ("afaire", "README + commande de test, rendu.json (C++, make, commande, video)"),
    ]),
]

# --------------------------------------------------------------------------
# Mini ecrivain PDF
# --------------------------------------------------------------------------
W, H = 595, 842          # A4 en points
MARGE = 50
COULEURS = {             # (remplissage case, libelle)
    "fait":    ((0.18, 0.55, 0.34), "fait"),
    "encours": ((0.90, 0.60, 0.10), "en cours"),
    "afaire":  (None,               "a faire"),
    "bloque":  ((0.80, 0.20, 0.20), "bloque"),
}


def esc(s):
    return s.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")


class Page:
    def __init__(self):
        self.ops = []
        self.y = H - MARGE

    def texte(self, x, y, s, taille=10, gras=False, gris=0.0):
        police = "F2" if gras else "F1"
        self.ops.append(f"BT {gris} g /{police} {taille} Tf {x} {y} Td ({esc(s)}) Tj ET")

    def rect(self, x, y, w, h, rempli=None, trait=(0.3, 0.3, 0.3)):
        if rempli:
            self.ops.append(f"{rempli[0]} {rempli[1]} {rempli[2]} rg {x} {y} {w} {h} re f")
        self.ops.append(f"{trait[0]} {trait[1]} {trait[2]} RG 0.8 w {x} {y} {w} {h} re S")

    def ligne(self, x1, y1, x2, y2, gris=0.7):
        self.ops.append(f"{gris} G 0.5 w {x1} {y1} m {x2} {y2} l S")

    def flux(self):
        return "\n".join(self.ops).encode("cp1252")


def construire():
    pages = [Page()]

    def p():
        return pages[-1]

    def place(hauteur):
        if p().y - hauteur < MARGE:
            pages.append(Page())

    # En-tete
    p().texte(MARGE, p().y, "Robot de reconfort - suivi d'Eliott", 20, gras=True)
    p().y -= 20
    p().texte(MARGE, p().y, f"Partie A : monde & validation   |   mise a jour : {MAJ}", 10, gris=0.4)
    p().y -= 22

    # Avancement global
    tout = [st for _, items in SECTIONS for st, _ in items]
    faits = tout.count("fait")
    p().texte(MARGE, p().y, f"Avancement global : {faits} / {len(tout)} taches", 11, gras=True)
    p().y -= 14
    larg = W - 2 * MARGE
    p().rect(MARGE, p().y, larg, 8)
    if faits:
        p().rect(MARGE, p().y, larg * faits / len(tout), 8, rempli=COULEURS["fait"][0])
    p().y -= 26

    # Prochaine etape
    hb = 18 + 13 * len(PROCHAINE)
    p().rect(MARGE, p().y - hb + 12, larg, hb, rempli=(0.93, 0.95, 1.0), trait=(0.4, 0.5, 0.8))
    p().texte(MARGE + 8, p().y, "Prochaine etape", 11, gras=True)
    p().y -= 15
    for l in PROCHAINE:
        p().texte(MARGE + 8, p().y, l, 10)
        p().y -= 13
    p().y -= 16

    # Sections
    for titre, items in SECTIONS:
        place(40)
        n = sum(1 for st, _ in items if st == "fait")
        p().texte(MARGE, p().y, titre, 13, gras=True)
        p().texte(W - MARGE - 40, p().y, f"{n}/{len(items)}", 11, gras=True, gris=0.4)
        p().y -= 6
        p().ligne(MARGE, p().y, W - MARGE, p().y)
        p().y -= 15
        for st, desc in items:
            lignes = textwrap.wrap(desc, 82)
            place(14 * len(lignes) + 4)
            couleur, libelle = COULEURS[st]
            p().rect(MARGE + 2, p().y - 1, 9, 9, rempli=couleur)
            for i, l in enumerate(lignes):
                p().texte(MARGE + 20, p().y, l, 10, gris=0.45 if st == "fait" else 0.0)
                if i == 0:
                    p().texte(W - MARGE - 50, p().y, libelle, 8, gris=0.4)
                p().y -= 13
            p().y -= 3
        p().y -= 12

    # Legende
    place(20)
    x = MARGE
    for st in ("fait", "encours", "afaire", "bloque"):
        couleur, libelle = COULEURS[st]
        p().rect(x, p().y - 1, 9, 9, rempli=couleur)
        p().texte(x + 14, p().y, libelle, 9, gris=0.3)
        x += 90
    return pages


def ecrire_pdf(pages, chemin):
    objets = []  # contenus des objets 1..N

    def ajoute(contenu):
        objets.append(contenu)
        return len(objets)

    cat = ajoute(None)
    arbre = ajoute(None)
    f1 = ajoute(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
    f2 = ajoute(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>")
    kids = []
    for pg in pages:
        data = pg.flux()
        c = ajoute(b"<< /Length %d >>\nstream\n" % len(data) + data + b"\nendstream")
        kids.append(ajoute((f"<< /Type /Page /Parent {arbre} 0 R /MediaBox [0 0 {W} {H}] "
                            f"/Resources << /Font << /F1 {f1} 0 R /F2 {f2} 0 R >> >> "
                            f"/Contents {c} 0 R >>").encode()))
    objets[cat - 1] = f"<< /Type /Catalog /Pages {arbre} 0 R >>".encode()
    objets[arbre - 1] = (f"<< /Type /Pages /Kids [{' '.join(f'{k} 0 R' for k in kids)}] "
                         f"/Count {len(kids)} >>").encode()

    sortie = bytearray(b"%PDF-1.4\n")
    offsets = []
    for i, o in enumerate(objets, 1):
        offsets.append(len(sortie))
        sortie += b"%d 0 obj\n" % i + o + b"\nendobj\n"
    xref = len(sortie)
    sortie += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objets) + 1)
    for off in offsets:
        sortie += b"%010d 00000 n \n" % off
    sortie += b"trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (
        len(objets) + 1, cat, xref)
    with open(chemin, "wb") as f:
        f.write(sortie)


if __name__ == "__main__":
    ici = os.path.dirname(os.path.abspath(__file__))
    ecrire_pdf(construire(), os.path.join(ici, "SUIVI_ELIOTT.pdf"))
    print("suivi/SUIVI_ELIOTT.pdf ecrit")
