use geo::{Contains};
use las::Point;
use crate::{las_wrapper::{LasReader, LasWriter}, dataframe_wrapper::{DataFrameWrapper, FilterByPosition, Filter, FilterByTile}};
use std::{fs, path::PathBuf};

pub struct Program {
    reader: Option<LasReader>,
    writer: Option<LasWriter>,
    dataframe: Option<DataFrameWrapper>,
    in_folder: String,
    out_folder: String,
    dataframe_folder: String,
    file_name: Option<String>,
}

impl Program {

    pub fn new(in_folder: &str, out_folder: &str, dataframe_folder: &str) -> Self {
        Self {
            reader: None,
            writer: None,
            dataframe: None,
            in_folder: in_folder.to_string(),
            out_folder: out_folder.to_string(),
            dataframe_folder: dataframe_folder.to_string(),
            file_name: None
        }
    }

    pub fn run(&mut self) {
        if let Ok(entries) = fs::read_dir(&self.in_folder) {

            for entry in entries {
                if let Ok(entry) = entry {
                    let path = entry.path();

                    if path.is_file() {
                        self.load(path);
                        self.process();
                        self.clear();
                    }
                }
            }
        }
    }

    pub fn load(&mut self, path: PathBuf) {
        let in_path = path.to_str().unwrap();
        let file_name = path.file_name().unwrap().to_str().unwrap();
        let out_path = self.out_folder.clone() + file_name;

        self.file_name = Some(file_name.to_string());
        self.reader = Some(LasReader::new(in_path).unwrap());
        self.writer = Some(LasWriter::new(&out_path, self.reader.as_ref().unwrap().get_header().unwrap()).unwrap());
        self.dataframe = Some(DataFrameWrapper::new(&self.dataframe_folder, b';').unwrap());
    }

    pub fn process(&mut self) {
        let reader = self.reader.as_mut().unwrap();
        let points = reader.get_points();
        let mut out_points: Vec<Point>= Vec::new();

        let tile = String::from(self.file_name.as_ref().unwrap());
        let tile_discr = tile.trim_end_matches(".las");

        let filter_by_tile = FilterByTile { tile: tile_discr.to_string() };
        let mut filtered_df = filter_by_tile.apply(&self.dataframe.as_ref().unwrap()).unwrap();

        let polygons = filtered_df.get_polygons().unwrap();
        let elevations = filtered_df.get_elevations().unwrap();
        let tollerances = filtered_df.get_tollerances().unwrap();

        let start = std::time::Instant::now();

        for point in points.unwrap() {
            let mut point = point.unwrap();

            if point.nir.is_none() {
                point.nir = Some(0);
            } 

            for (i, polygon) in polygons.iter().enumerate() {
                if polygon.contains(&geo::Point::new(point.x, point.y)) {
                    if point.z >= elevations[i] && point.z <= elevations[i] + tollerances[i] {
                        out_points.push(point);
                        break;
                    }
                }
            }
        }

        self.writer.as_mut().unwrap().write_points(out_points);

        let duration = start.elapsed();
        println!("Time elapsed in expensive_function() is: {:?}", duration);
    }

    pub fn clear(&mut self) {
        self.writer.as_mut().unwrap().close();
    }

}