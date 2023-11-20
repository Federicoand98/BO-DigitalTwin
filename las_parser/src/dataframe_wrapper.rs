use std::error::Error;

use geo::{Polygon, Coordinate, Coord};
use polars::prelude::*;

#[derive(Clone)]
pub struct DataFrameWrapper {
    dataframe: DataFrame
}

pub trait Filter {
    fn apply(&self, df: &DataFrameWrapper) -> Result<DataFrameWrapper, Box<dyn Error>>;
}

pub struct FilterByPosition {
    pub min_x: f64,
    pub min_y: f64
}

impl Filter for FilterByPosition {

    fn apply(&self, df: &DataFrameWrapper) -> Result<DataFrameWrapper, Box<dyn Error>> {
        let mut new_df = df.clone();
        
        let range_x = (self.min_x - 25.0, self.min_x + 525.0);
        let range_y = (self.min_y - 25.0, self.min_y + 525.0);

        let mask_x = new_df.dataframe.column("Geo Point UTM X")?.f64().unwrap().gt(range_x.0) & new_df.dataframe.column("Geo Point UTM X")?.f64().unwrap().lt(range_x.1);
        let mask_y = new_df.dataframe.column("Geo Point UTM Y")?.f64().unwrap().gt(range_y.0) & new_df.dataframe.column("Geo Point UTM Y")?.f64().unwrap().lt(range_y.1);
        let mask = mask_x & mask_y;

        new_df.dataframe = new_df.dataframe.filter(&mask)?;

        Ok(new_df)
    }
}

impl DataFrameWrapper {

    pub fn new(file_path: &str, separator: u8) -> Result<Self, Box<dyn Error>> {
        let csv = CsvReader::from_path(file_path)?
            .has_header(true)
            .with_separator(separator)
            .finish()
            .unwrap();

        Ok(Self { dataframe: csv })
    }

    pub fn set_dataframe(&mut self, df: DataFrame) {
        self.dataframe = df;
    }

    pub fn get_elevations(&mut self) -> Result<Vec<f64>, Box<dyn Error>> {
        let elevations: Vec<f64> = self.dataframe.column("QUOTA_GRONDA")?.clone().f64().unwrap().into_iter().map(|v| v.unwrap()).collect();
        Ok(elevations)
    }

    pub fn get_tollerances(&mut self) -> Result<Vec<f64>, Box<dyn Error>> {
        let tollerances: Vec<f64> = self.dataframe.column("Toll_Tetto")?.clone().f64().unwrap().into_iter().map(|v| v.unwrap()).collect();
        Ok(tollerances)
    }

    pub fn get_geo_shapes(&mut self) -> Result<Vec<Vec<Vec<String>>>, Box<dyn Error>> {
        let geo_shapes = self.dataframe.column("Geo Shape UTM_OFF")?.clone();
        
        let geo_shapes: Vec<Vec<Vec<String>>> = geo_shapes.utf8().unwrap()
            .into_iter()
            .map(|s| {
                s.unwrap().trim_matches(|c| c == '[' || c == ']')
                    .split("], [")
                    .map(|s| s.split(", ").map(String::from).collect())
                    .collect()
            })
            .collect();

        Ok(geo_shapes)
    }

    pub fn get_polygons(&mut self) -> Result<Vec<Polygon<f64>>, Box<dyn Error>> {
        let mut polygons: Vec<Polygon<f64>> = Vec::new();
        let geo_shapes = self.get_geo_shapes().unwrap();

        for shape in geo_shapes {
            let mut points: Vec<Coord<f64>> = Vec::new();

            for point in shape {
                let x: f64 = point[0].parse().unwrap();
                let y: f64 = point[1].parse().unwrap();

                points.push(Coord { x, y });
            }

            let polygon = Polygon::new(points.into(), vec![]);
            polygons.push(polygon);
        }

        Ok(polygons)
    }
}