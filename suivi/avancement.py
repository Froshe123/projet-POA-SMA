#!/usr/bin/env python3
"""Génère suivi/AVANCEMENT.pdf (Eliott + Axel) à partir des listes ci-dessous.
Pour mettre à jour : changer l'état (FAIT / EN COURS / A FAIRE) et la note,
puis relancer :  python3 suivi/avancement.py   (nécessite reportlab)."""
import datetime, os
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import cm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle

FAIT, COURS, TODO, BLOQUE = "FAIT", "EN COURS", "A FAIRE", "BLOQUÉ"

# (section, tâche, état, note)
AXEL = [
 ("Carte & mémoire", "Carte mentale (hypothèse optimiste)", FAIT, "carte_robot.cpp/.hpp : INCONNUE/LIBRE/OCCUPEE, mise à jour par case"),
 ("Carte & mémoire", "Mémoire du robot (struct Memoire)", COURS, "memoire.hpp écrit (carte, robot, sélecteur, vu/contenu) ; memoire.cpp à écrire"),
 ("Carte & mémoire", "Mise à jour à chaque perception", COURS, "noter_perception / oublier_armoire déclarés, non implémentés"),
 ("Planification", "BFS + file (plus court chemin)", FAIT, "graphe.cpp : prochain_pas, file écrite à la main, sans lib"),
 ("Planification", "Replanification à chaque pas", FAIT, "un pas puis recalcul ; PAS_DE_CHEMIN si inaccessible"),
 ("Décision", "Lecture du dictionnaire (6.1)", FAIT, "dictionnaire.cpp : premier mot reconnu, sinon indéterminée"),
 ("Décision", "Choix du casier (6.2) : 15 candidats triés", FAIT, "choix_casier.cpp : casiers_a_essayer, prochain_casier, étiquettes de repli"),
 ("Décision", "Itinéraire du sélecteur (annexe C)", FAIT, "pas_du_selecteur : vertical d'abord + bouclage des colonnes (C.3, accepté par C.4)"),
 ("Décision", "Enchaînement des 4 étapes par demande", TODO, "agent.cpp : classe Agent de interface.hpp (decider, appliquer, bilan)"),
 ("Décision", "Gestion des échecs (6.3)", TODO, "résident inaccessible, armoire vide, émotion indéterminée"),
 ("Tests", "Test 1 : planification petite grille", FAIT, "tests.cpp : mini-grille B.3, chemin SSEENE, puis X si bouché"),
 ("Tests", "Test 2 : replanification (mur découvert)", FAIT, "tests.cpp : murs vus en (1,2), nouveau chemin OSSEENE"),
 ("Tests", "Test 3 : lecture du dictionnaire", FAIT, "tests.cpp : 8 émotions + 2 mots connus + aucun mot"),
 ("Tests", "Test 5 : règle de repli", FAIT, "tests.cpp : exact, intensité, voisine, armoire vide, ordre respecté"),
]

# Partie d'Eliott : lue dans son generer_suivi.py (il reste la source de vérité)
import importlib.util
_spec = importlib.util.spec_from_file_location("generer_suivi", os.path.join(os.path.dirname(os.path.abspath(__file__)), "generer_suivi.py"))
_el = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_el)
_ETAT = {"fait": FAIT, "encours": COURS, "afaire": TODO, "bloque": BLOQUE}
_rows = [(titre.split(". ", 1)[-1], d, _ETAT[st], "") for titre, items in _el.SECTIONS for st, d in items]
_n_ens = len(_el.SECTIONS[-1][1])
ELIOTT = _rows[:-_n_ens]
ENSEMBLE = _rows[-_n_ens:]
PROCHAINE_ELIOTT = " ".join(_el.PROCHAINE)

COL = {FAIT: colors.HexColor("#2e7d32"), COURS: colors.HexColor("#ef8f00"), TODO: colors.HexColor("#9e9e9e"), BLOQUE: colors.HexColor("#c62828")}
PRET = {FAIT: 1.0, COURS: 0.5, TODO: 0.0, BLOQUE: 0.0}
LARG = 17.8 * cm


def pourcentage(taches):
    return round(100 * sum(PRET[t[2]] for t in taches) / len(taches))


def section(el, ss, n, titre, role, taches):
    nb = {k: sum(1 for t in taches if t[2] == k) for k in (FAIT, COURS, TODO, BLOQUE)}
    pct = pourcentage(taches)
    el.append(Paragraph(titre, ss["Heading1"]))
    el.append(Paragraph("%s — <b>%d %%</b> : %d fait, %d en cours, %d à faire, %d bloqué" % (role, pct, nb[FAIT], nb[COURS], nb[TODO], nb[BLOQUE]), ss["Normal"]))
    barre = Table([["", ""]], colWidths=[LARG * pct / 100 or 0.01, LARG * (100 - pct) / 100 or 0.01], rowHeights=[0.35 * cm])
    barre.setStyle(TableStyle([("BACKGROUND", (0, 0), (0, 0), COL[FAIT]), ("BACKGROUND", (1, 0), (1, 0), colors.HexColor("#e0e0e0"))]))
    el += [Spacer(1, 0.2 * cm), barre, Spacer(1, 0.3 * cm)]
    rows = [["Section", "Tâche", "État", "Détail"]]
    for s, t, e, note in taches:
        rows.append([Paragraph(s, n), Paragraph(t, n), Paragraph('<font color="white"><b>%s</b></font>' % e, n), Paragraph(note, n)])
    tb = Table(rows, colWidths=[2.8 * cm, 7.4 * cm, 1.9 * cm, 5.7 * cm], repeatRows=1)
    st = [("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#263238")), ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
          ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#bdbdbd")), ("VALIGN", (0, 0), (-1, -1), "MIDDLE")]
    for i, t in enumerate(taches, 1):
        st.append(("BACKGROUND", (2, i), (2, i), COL[t[2]]))
    tb.setStyle(TableStyle(st))
    el.append(tb)


def main():
    ss = getSampleStyleSheet()
    n = ParagraphStyle("n", parent=ss["BodyText"], fontSize=8.5, leading=10.5)
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "AVANCEMENT.pdf")
    doc = SimpleDocTemplate(out, pagesize=A4, leftMargin=1.6 * cm, rightMargin=1.6 * cm, topMargin=1.5 * cm, bottomMargin=1.5 * cm,
                            title="Avancement - Projet Robot de Reconfort", author="Eliott & Axel")
    total = pourcentage(AXEL + ELIOTT + ENSEMBLE)
    el = [Paragraph("Robot de Réconfort — avancement du binôme", ss["Title"]),
          Paragraph("Mis à jour le %s — projet global : <b>%d %%</b>" % (datetime.date.today().strftime("%d/%m/%Y"), total), ss["Normal"])]
    section(el, ss, n, "Axel (B)", "Agent &amp; décision", AXEL)
    el.append(Spacer(1, 0.5 * cm))
    section(el, ss, n, "Eliott (A)", "Monde &amp; validation", ELIOTT)
    el.append(Spacer(1, 0.5 * cm))
    section(el, ss, n, "Ensemble", "À la fin, à deux", ENSEMBLE)
    el += [Spacer(1, 0.5 * cm), Paragraph(
        "<b>Prochaines étapes</b><br/>Axel : memoire.cpp et choix_casier.cpp, puis itinéraire du sélecteur et agent.cpp.<br/>"
        "Eliott : " + PROCHAINE_ELIOTT, ss["Normal"])]
    doc.build(el)
    print(out, "Axel", pourcentage(AXEL), "% / Eliott", pourcentage(ELIOTT), "%")


main()
