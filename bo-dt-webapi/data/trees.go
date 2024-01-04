package data

import (
	"fmt"
	"io/ioutil"
	"log"
	"strings"

	"github.com/go-gota/gota/dataframe"
)

func GetTrees() []Tree {
	f, err := ioutil.ReadFile("./alberi-manutenzioni-filtered.csv")
	if err != nil {
		log.Fatal(err)
	}

	dataframe := dataframe.ReadCSV(strings.NewReader(string(f)))
	fmt.Println(dataframe.Elem(dataframe.Nrow()-1, 0))

	var trees = make([]Tree, dataframe.Nrow())

	for i := 0; i < dataframe.Nrow(); i++ {
		trees[i] = Tree{
			Row:                 dataframe.Elem(i, 0).String(),
			ProgressivoPianta:   dataframe.Elem(i, 1).Float(),
			CodiceAlbero:        dataframe.Elem(i, 2).String(),
			PiantaIsolata:       dataframe.Elem(i, 3).String(),
			SpecieArborea:       dataframe.Elem(i, 4).String(),
			Dimora:              dataframe.Elem(i, 5).String(),
			Irrigazione:         dataframe.Elem(i, 6).String(),
			AlberoDiPregio:      dataframe.Elem(i, 7).String(),
			ClasseCirconferenza: dataframe.Elem(i, 8).String(),
			DataInserimento:     dataframe.Elem(i, 9).String(),
			DataUltimaModifica:  dataframe.Elem(i, 10).String(),
			DistanzaDaPalazzo:   dataframe.Elem(i, 11).String(),
			DataImpianto:        dataframe.Elem(i, 12).String(),
			ZonaProssimita:      dataframe.Elem(i, 13).String(),
			AreaStatistica:      dataframe.Elem(i, 14).String(),
			X:                   dataframe.Elem(i, 15).Float(),
			Y:                   dataframe.Elem(i, 16).Float(),
			ClasseArborea:       dataframe.Elem(i, 17).String(),
			ClasseAltezza:       dataframe.Elem(i, 18).String(),
			QuotaTerreno:        dataframe.Elem(i, 19).Float(),
		}
	}

	return trees
}
