package data

type Tree struct {
	Row                 string  `json:"row"`
	ProgressivoPianta   float64 `json:"progressivoPianta"`
	CodiceAlbero        string  `json:"codiceAlbero"`
	PiantaIsolata       string  `json:"piantaIsolata"`
	SpecieArborea       string  `json:"specieArborea"`
	Dimora              string  `json:"dimora"`
	Irrigazione         string  `json:"irrigazione"`
	AlberoDiPregio      string  `json:"alberoDiPregio"`
	ClasseCirconferenza string  `json:"classeCirconferenza"`
	DataInserimento     string  `json:"dataInserimento"`
	DataUltimaModifica  string  `json:"dataUltimaModifica"`
	DistanzaDaPalazzo   string  `json:"distanzaDaPalazzo"`
	DataImpianto        string  `json:"dataImpianto"`
	ZonaProssimita      string  `json:"zonaProssimita"`
	AreaStatistica      string  `json:"areaStatistica"`
	X                   float64 `json:"x"`
	Y                   float64 `json:"y"`
	ClasseArborea       string  `json:"classeArborea"`
	ClasseAltezza       string  `json:"classeAltezza"`
	QuotaTerreno        float64 `json:"quotaTerreno"`
}
