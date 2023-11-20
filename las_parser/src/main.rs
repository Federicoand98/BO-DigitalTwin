#![allow(dead_code, unused_variables)]

pub mod las_wrapper;
pub mod dataframe_wrapper;
pub mod program;

use std::error::Error;
use program::Program;

fn main() -> Result<(), Box<dyn Error>> {

    let mut program = Program::new(
        "assets/raw_point_cloud/",
        "assets/filtered_point_cloud/",
        "assets/filtered_buildings_extra.csv");

    program.run();

    // let mut df = read_csv("assets/filtered_buildings_withTol.csv").unwrap();
    // let mut reader = Reader::from_path("assets/32_684000_4930000.las").expect("Failed to read file");
    // let mut writer = Writer::from_path("assets/new_las.las", reader.header().clone()).expect("Failed to create file");

    // let min_x: f64 = 684000.0;
    // let min_y: f64 = 4930000.0;

    // df = df.add_aux_columns().unwrap();

    // df = df.filter_shape_by_range(min_x, min_y).unwrap();

    // let polygons = build_polygons(&df);

    // let elevations: Vec<f64> = df.column("QUOTA_GRONDA")?.clone().f64().unwrap().into_iter().map(|v| v.unwrap()).collect();
    // let tollerances: Vec<f64> = df.column("Toll_Tetto")?.clone().f64().unwrap().into_iter().map(|s| s.unwrap()).collect();

    // filter_roofs(reader, writer, polygons?, elevations, tollerances);

    // TODO: aggiungere X Y direttamente nel dataframe originale


    Ok(())
}

// trait ShapeExtension {
//     fn add_aux_columns(&self) -> Result<DataFrame, Box<dyn Error>>;
//     fn filter_shape_by_range(&self, min_x: f64, min_y: f64) -> Result<DataFrame, Box<dyn Error>>;
// }

// impl ShapeExtension for DataFrame {

//     fn add_aux_columns(&self) -> Result<DataFrame, Box<dyn Error>> {
//         let mut df = self.clone();

//         let geo_points_utm = df.column("Geo Point UTM")?.clone();
//         let geo_points_utm = geo_points_utm.utf8().unwrap();
//         let geo_points_utm: Vec<Vec<&str>>= geo_points_utm
//             .into_iter()
//             .map(|s| s.unwrap().trim_matches(|c| c == '[' || c == ']').split(", ").collect())
//             .collect();

//         let x: Vec<f64> = geo_points_utm.iter().map(|v| v[0].parse().unwrap()).collect();
//         let y: Vec<f64> = geo_points_utm.iter().map(|v| v[1].parse().unwrap()).collect();

//         df = df.with_column(Series::new("X", x)).unwrap().clone();
//         df = df.with_column(Series::new("Y", y)).unwrap().clone();

//         Ok(df)
//     }
    
//     fn filter_shape_by_range(&self, min_x: f64, min_y: f64) -> Result<DataFrame, Box<dyn Error>> {
//         let df = self.clone();

//         let range_x = (min_x - 25.0, min_x + 525.0);
//         let range_y = (min_y - 25.0, min_y + 525.0);

//         let mask_x = df.column("X")?.f64().unwrap().gt(range_x.0) & df.column("X")?.f64().unwrap().lt(range_x.1);
//         let mask_y = df.column("Y")?.f64().unwrap().gt(range_y.0) & df.column("Y")?.f64().unwrap().lt(range_y.1);
//         let mask = mask_x & mask_y;

//         let df_filtered = df.filter(&mask).unwrap();

//         Ok(df_filtered)
//     }
// }

// fn rgb_filter(point: &Point, tollerance: u16) -> bool {
//     let color = point.color.unwrap();
//     let avg_rb = (color.red + color.blue) / 2;

//     if color.green > avg_rb + tollerance {
//         return false
//     }

//     true
// }

// fn filter_roofs(mut reader: Reader<'_>, mut writer: Writer<BufWriter<File>>, polygons: Vec<Polygon<f64>>, elevations: Vec<f64>, tollerances: Vec<f64>) {
//     let start = std::time::Instant::now();

//     for wrapped_point in reader.points() {
//         let mut point = wrapped_point.unwrap();
//         // let mut found = false;

//         if point.nir.is_none() {
//             point.nir = Some(0);
//         }

//         for (i, polygon) in polygons.iter().enumerate() {
//             if polygon.contains(&geo::Point::new(point.x, point.y)) {
//                 if point.z >= elevations[i] && point.z <= elevations[i] + tollerances[i] {
//                     // found = true;

//                     // if rgb_filter(&point, ) {
//                     //     writer.write(point).expect("Failed to write point");
//                     //     break;
//                     // }

//                     // if !point.nir.is_none() {
//                     //     let ndvi: f64 = (point.nir.unwrap() as f64 - point.color.unwrap().red as f64) / (point.nir.unwrap() as f64 + point.color.unwrap().red as f64);

//                     //     if ndvi >= 1.0 {
//                     //         writer.write(point).expect("Failed to write point");
//                     //         break;
//                     //     }
//                     // }

//                     writer.write(point).expect("Failed to write point");
//                     break;
//                 }
//             }
//         }

//         // if !found {
//         //     point.z = 0.0;
//         // }
//     }

//     writer.close().expect("Failed to close writer");

//     let duration = start.elapsed();
//     println!("Time elapsed in expensive_function() is: {:?}", duration);
// }

// fn build_polygons(dataframe: &DataFrame) -> Result<Vec<Polygon<f64>>, Box<dyn Error>> {
//     let geo_shapes = dataframe.column("Geo Shape UTM_OFF")?.clone();
//     let geo_shapes = geo_shapes.utf8().unwrap();
//     let geo_shapes: Vec<Vec<Vec<&str>>> = geo_shapes
//         .into_iter()
//         .map(|s| s.unwrap().trim_matches(|c| c == '[' || c == ']').split("], [").map(|s| s.split(", ").collect()).collect())
//         .collect();

//     let mut polygons: Vec<Polygon<f64>> = Vec::new();

//     for shape in geo_shapes {
//         let mut points: Vec<Coordinate<f64>> = Vec::new();

//         for point in shape {
//             let x: f64 = point[0].parse().unwrap();
//             let y: f64 = point[1].parse().unwrap();

//             points.push(Coordinate { x, y });
//         }

//         let polygon = Polygon::new(points.into(), vec![]);
//         polygons.push(polygon);
//     }

//     Ok(polygons)
// }

// fn read_csv(path: &str) -> PolarsResult<DataFrame> {
//     CsvReader::from_path(Path::new(path))?
//             .has_header(true)
//             .with_separator(b';')
//             .finish()
// }



