#include "gtest/gtest.h"
#include "../fsm.h"
#include "../lexer.h"
#include "../parser.h"
#include "../ast.h"
#include <memory>
#include <string>

class FSMTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    std::string exampleLatexWithAuthorAffiliation = R"(
\documentclass[twocolumn]{aastex631}
%\documentclass[twocolumn,linenumbers]{aastex631}

\newcommand{\vdag}{(v)^\dagger}
\newcommand\aastex{AAS\TeX}
\newcommand\latex{La\TeX}

\usepackage{amsmath,amstext}
\usepackage[T1]{fontenc}
\usepackage{multirow}
\usepackage[utf8]{inputenc} 
\usepackage{float}
\usepackage{graphicx}
\newcommand{\unit}[1]{\ensuremath{\, \mathrm{#1}}}
\usepackage{tabularx} 

\usepackage{threeparttable}

\shorttitle{TOI-1266 TTVs}
\shortauthors{Greklek-McKeon et al.}

\begin{document}

\title{Tidally Heated Sub-Neptunes, Refined Planetary Compositions, and Confirmation of a Third Planet in the TOI-1266 System}


\correspondingauthor{Michael Greklek-McKeon}
\email{michael@caltech.edu}

\author[0000-0002-0371-1647]{Michael Greklek-McKeon}
\affiliation{Division of Geological and Planetary Sciences, California Institute of Technology, Pasadena, CA 91125, USA}

\author[0000-0003-2527-1475]{Shreyas Vissapragada}
\affiliation{Carnegie Science Observatories, Pasadena, CA 91101, USA}

\author[0000-0002-5375-4725]{Heather A. Knutson}
\affiliation{Division of Geological and Planetary Sciences, California Institute of Technology, Pasadena, CA 91125, USA}

\author[0000-0002-4909-5763]{Akihiko Fukui}
\affiliation{Komaba Institute for Science, The University of Tokyo, Tokyo 153-8902, Japan}
\affiliation{Instituto de Astrof\'{i}sica de Canarias (IAC), 38205 La Laguna, Tenerife, Spain}

\author[0000-0001-9518-9691]{Morgan Saidel}
\affiliation{Division of Geological and Planetary Sciences, California Institute of Technology, Pasadena, CA 91125, USA}

\author[0000-0002-0672-9658]{Jonathan Gomez Barrientos}
\affiliation{Division of Geological and Planetary Sciences, California Institute of Technology, Pasadena, CA 91125, USA}

\author[0000-0002-1422-4430]{W. Garrett Levine}
\affiliation{Department of Astronomy, Yale University, New Haven, CT 06511, USA}

\author[0000-0003-0012-9093]{Aida Behmard}
\affiliation{American Museum of Natural History, 200 Central Park West, New York, NY 10024}

\author[0000-0002-7094-7908]{Konstantin Batygin}
\affiliation{Division of Geological and Planetary Sciences, California Institute of Technology, Pasadena, CA 91125, USA}

\author[0000-0003-1728-8269]{Yayaati Chachan}
\affiliation{Department of Physics, McGill University
3600 Rue University, Montreal, Quebec H3A 2T8, Canada}
\end{document}
	)";

    std::string exampleLatexWithAffiliationRefs = R"(
\documentclass{aa}  

%
\usepackage[dvipsnames]{xcolor}
\usepackage{graphicx}
\usepackage{array}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\usepackage{txfonts}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\usepackage{booktabs}
\usepackage{placeins}
% \usepackage{lscape}
% \usepackage{lineno}
% \linenumbers
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% \usepackage[options]{hyperref}
% \usepackage[pdftex]{hyperref}
% To add links to your PDF file, use the package "hyperref"
% with options according to your LaTeX or PDFLaTeX drivers.
%


\newcommand{\feI}{\ion{Fe}{i}}
\newcommand{\feII}{\ion{Fe}{ii}}
% \newcommand{\snr}{S/N$_\mathrm{ratio}$}
\newcommand{\snr}{S/N}
\newcommand{\planet}{WASP-76\,b}
\newcommand{\hoststar}{WASP-76}
\newcommand{\vsys}{v$_\mathrm{sys}$}
\newcommand{\kpvsys}{K$_\mathrm{p}$\,-\,v$_\mathrm{sys}$}
\newcommand{\kp}{K$_\mathrm{p}$}


\begin{document} 


   \title{ESPRESSO reveals blueshifted neutral iron emission\\
   lines on the dayside of WASP-76\,b\,\thanks{Based on Guaranteed Time Observations collected at the European Southern Observatory under ESO programmes 1104.C-0350(U) and 110.24CD.004 by the ESPRESSO Consortium.}}

   % \subtitle{}

\author{
         \mbox{A. R. Costa Silva\inst{\ref{caup},\ref{dfa},\ref{geneve}}}
         \and
         \mbox{O. D. S. Demangeon\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{N. C. Santos\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{D. Ehrenreich\inst{\ref{geneve},\ref{cvu}}}
         \and
         \mbox{C. Lovis\inst{\ref{geneve}}}
         \and
         \mbox{H. Chakraborty\inst{\ref{geneve}}}
         \and
         \mbox{M. Lendl\inst{\ref{geneve}}}
         \and
         \mbox{F. Pepe\inst{\ref{geneve}}}
         \and
         \mbox{S. Cristiani\inst{\ref{inaf_trieste}}}
         \and
         \mbox{R. Rebolo\inst{\ref{iac},\ref{laguna},\ref{csic}}}
         \and
         \mbox{M. R. Zapatero-Osorio\inst{\ref{cab_madrid_mrzo}}}
         \and
         \mbox{V. Adibekyan\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{Y. Alibert\inst{\ref{bern}}}
         \and
         \mbox{R. Allart\inst{\ref{montreal},\ref{geneve}}\thanks{Trottier Postdoctoral Fellow}}
         \and
         \mbox{C. Allende Prieto\inst{\ref{iac},\ref{laguna}}}
         \and
         \mbox{T. Azevedo Silva\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{F. Borsa\inst{\ref{inaf_brera}}}
         \and
         \mbox{V. Bourrier\inst{\ref{geneve}}}
         \and
         \mbox{E. Cristo\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{P. Di Marcantonio\inst{\ref{inaf_trieste}}}
         \and
         \mbox{E. Esparza-Borges\inst{\ref{iac},\ref{laguna}}}
         \and
         \mbox{P. Figueira\inst{\ref{geneve},\ref{caup}}}
         \and
         \mbox{J. I. Gonz\'alez Hern\'andez\inst{\ref{iac},\ref{laguna}}}
         \and
         \mbox{E. Herrero-Cisneros\inst{\ref{cab_madrid}}}
         \and
         \mbox{G. Lo Curto\inst{\ref{eso}}}
         \and
         \mbox{C. J. A. P. Martins\inst{\ref{caup},\ref{caup_solo}}}
         \and
         \mbox{A. Mehner\inst{\ref{eso}}} 
         \and
         \mbox{N. J. Nunes\inst{\ref{fcul}}}
         \and
         \mbox{E. Palle\inst{\ref{iac},\ref{laguna}}}
         \and
         \mbox{S. Pelletier\inst{\ref{geneve}}}
         \and
         \mbox{J. V. Seidel\inst{\ref{eso}}}
         \and
         \mbox{A. M. Silva\inst{\ref{caup},\ref{dfa}}}
         \and
         \mbox{S. G. Sousa\inst{\ref{caup}}}
         \and 
         \mbox{A. Sozzetti\inst{\ref{inaf_torino}}}
         \and
         \mbox{M. Steiner\inst{\ref{geneve}}}      
         \and
         \mbox{A. Su{\'a}rez Mascare{\~n}o\inst{\ref{iac},\ref{laguna}}}
         \and
         \mbox{S. Udry\inst{\ref{geneve}}}
         }
         

   \institute{
             Instituto de Astrof\'isica e Ci\^encias do Espa\c{c}o, Universidade do Porto, CAUP, Rua das Estrelas, 4150-762 Porto, Portugal \label{caup}
             \and
             Departamento de F\'isica e Astronomia, Faculdade de Ci\^encias, Universidade do Porto, Rua do Campo Alegre, 4169-007 Porto, Portugal \label{dfa}
             \and
             Observatoire Astronomique de l'Universit\'e de Gen\`eve, Chemin Pegasi 51, 1290 Versoix, Switzerland \label{geneve} 
             \and
             Centre Vie dans l'Univers, Facult\'e des sciences, Universit\'e de Gen\`eve, Gen\`eve 4, Switzerland \label{cvu}
             \and
             INAF $-$ Osservatorio Astronomico di Trieste, via G. B. Tiepolo 11, I-34143, Trieste, Italy \label{inaf_trieste} 
             \and
             Instituto de Astrof{\'\i}sica de Canarias, c/ V\'ia L\'actea s/n, 38205 La Laguna, Tenerife, Spain \label{iac} 
             \and
             Departamento de Astrof\'{\i}sica, Universidad de La Laguna, 38206 La Laguna, Tenerife, Spain \label{laguna} 
             \and
             Consejo Superior de Investigaciones Cient\'{\i}cas, Spain \label{csic}
             \and
             Centro de Astrobiolog\'ia, CSIC-INTA, Camino Bajo del Castillo s/n, E-28692 Villanueva de la Ca\~nada, Madrid, Spain \label{cab_madrid_mrzo}
             \and
             Physics Institute, University of Bern, Sidlerstrasse 5, 3012 Bern, Switzerland \label{bern}
             \and
             D\'epartement de Physique, Institut Trottier de Recherche sur les Exoplan\`etes, Universit\'e de Montr\'eal, Montr\'eal, Qu\'ebec, H3T 1J4, Canada \label{montreal}
             \and
             INAF $-$ Osservatorio Astronomico di Brera, Via E. Bianchi 46, 23807 Merate (LC), Italy \label{inaf_brera}
             \and
             Centro de Astrobiolog\'ia, CSIC-INTA, Crta. Ajalvir km 4, E-28850 Torrej\'on de Ardoz, Madrid, Spain \label{cab_madrid}
             \and
             European Southern Observatory, Alonso de C\'ordova 3107, Vitacura, Regi\'on Metropolitana, Chile \label{eso} 
             \and
             Centro de Astrof\'{\i}sica da Universidade do Porto, Rua das Estrelas, 4150-762 Porto, Portugal \label{caup_solo}
             % \and
             % INAF $-$ Osservatorio Astronomico di Palermo, Piazza del Parlamento 1, I-90134 Palermo, Italy \label{inaf_palermo}     
             \and
             Instituto de Astrof\'isica e Ci\^encias do Espa\c{c}o, Faculdade de Ci\^encias da Universidade de Lisboa, Campo Grande, PT1749-016 Lisboa, Portugal \label{fcul}
             \and
             INAF $-$ Osservatorio Astrofisico di Torino, Via Osservatorio 20, 10025 Pino Torinese, Italy \label{inaf_torino}
            }
\end{document}
	)";
};

TEST_F(FSMTest, BasicAuthorAffiliationExtraction) {
    Lexer lexer(exampleLatexWithAuthorAffiliation);
    Parser parser(lexer);
    std::shared_ptr<AST> ast = parser.parseDocument();
    
    FSM fsm;
    nlohmann::json jsonDocument = fsm.chunkDocumentToJson(ast->root);
    
    ASSERT_TRUE(jsonDocument["document"]["metadata"]["authors"].is_array());
    ASSERT_EQ(jsonDocument["document"]["metadata"]["authors"].size(), 3);
    
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][0]["name"], "John Doe");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][1]["name"], "Jane Smith");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["name"], "James Johnson");

    ASSERT_TRUE(jsonDocument["document"]["metadata"]["authors"][0]["affiliations"].is_array());
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][0]["affiliations"][0], "University of Testland");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][1]["affiliations"][0], "University of Testland");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["affiliations"][0], "Department of Physics");
}

TEST_F(FSMTest, AffiliationReferenceExtraction) {
    Lexer lexer(exampleLatexWithAffiliationRefs);
    Parser parser(lexer);
    std::shared_ptr<AST> ast = parser.parseDocument();
    
    FSM fsm;
    nlohmann::json jsonDocument = fsm.chunkDocumentToJson(ast->root);
    
    ASSERT_TRUE(jsonDocument["document"]["metadata"]["authors"].is_array());
    ASSERT_EQ(jsonDocument["document"]["metadata"]["authors"].size(), 3);
    
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][0]["name"], "John Doe");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][1]["name"], "Jane Smith");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["name"], "James Johnson");

    ASSERT_TRUE(jsonDocument["document"]["metadata"]["authors"][0]["affiliations"].is_array());
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][0]["affiliations"][0], "Institute 1, Some University");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][1]["affiliations"][0], "Institute 2, Another University");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["affiliations"].size(), 2);
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["affiliations"][0], "Institute 1, Some University");
    EXPECT_EQ(jsonDocument["document"]["metadata"]["authors"][2]["affiliations"][1], "Institute 2, Another University");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

