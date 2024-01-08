use std::error::Error;
use std::{iter::*, result};
use std::f32::consts::PI;

use polars::prelude::*;
use plotters::style::colors::*;
use geo::{Polygon, Contains, Point, Coord};
use image::{GrayImage, Luma, ImageBuffer, RgbImage, GenericImageView, Rgb};
use imageproc::edges::canny;
use imageproc::corners::*;
use imageproc::morphology::*;
use imageproc::distance_transform::Norm;
use imageproc::hough::*;
use imageproc::map::map_colors;

pub mod drawer;
pub mod my_point;
pub mod grid;
pub mod clustering;
pub mod df_utils;
pub mod las_wrapper;

use crate::las_wrapper::{LasReader};
use crate::drawer::*;
use crate::my_point::*;
use crate::grid::*;
use crate::clustering::*;
use crate::df_utils::*;

pub fn fill_holes(mat: &mut Vec<Vec<f32>>, t: usize) {
    let mut kernel = Vec::new();

    for dx in -1..=1 {
        for dy in -1..=1 {
            if dx != 0 || dy != 0 {
                kernel.push((dx, dy));
            }
        }
    }

    let rows = mat.len();
    let cols = mat[0].len();

    for i in 0..rows {
        for j in 0..cols {
            if mat[i][j] == 0.0 {
                let mut count = 0;
                let mut sum = 0.0;

                for &(dx,dy) in &kernel {
                    if let Some(neighbour) = mat.get((i as i32 + dx) as usize).and_then(|row| row.get((j as i32 + dy) as usize)) {
                        if *neighbour > 0.0 {
                            count += 1;
                            sum += *neighbour;
                        }
                    }
                }

                if count > t {
                    mat[i][j] = sum / count as f32;
                }
            }
        }
    }
}

pub fn compute_gradient(v: &Vec<Vec<f32>>, dist: usize) -> Vec<Vec<(f32, f32)>> {
    let mut grad: Vec<Vec<(f32, f32)>> = vec![vec![(0.0, 0.0); v[0].len()]; v.len()];
    let k = dist / 2;

    for i in k..v.len()-k {
        for j in k..v[i].len()-k {
            let dx = (v[i+k][j] - v[i-k][j]) / (2.0 * k as f32);
            let dy = (v[i][j+k] - v[i][j-k]) / (2.0 * k as f32);
            grad[i][j] = (dx, dy);
        }
    }

    grad
}

pub fn compute_average_gradient(v: &Vec<Vec<f32>>, kernel_size: isize) -> Vec<Vec<f32>> {
    let mut grad: Vec<Vec<f32>> = vec![vec![0.0; v[0].len()]; v.len()];
    let k = kernel_size / 2;

    for i in (k as usize)..v.len()-(k as usize) {
        for j in (k as usize)..v[i].len()-(k as usize) {
            let mut sum = 0.0;
            let mut count = 0.0;
            for m in -k..=k {
                for n in -k..=k {
                    let i_index = i as isize + m;
                    let j_index = j as isize + n;
                    if i_index-k >= 0 && i_index+k < v.len() as isize && j_index-k >= 0 && j_index+k < v[i].len() as isize {
                        let dx = (v[(i_index+k) as usize][j] - v[(i_index-k) as usize][j]) / (2.0 * k as f32);
                        let dy = (v[i][(j_index+k) as usize] - v[i][(j_index-k) as usize]) / (2.0 * k as f32);
                        let magnitude = (dx.powi(2) + dy.powi(2)).sqrt();
                        sum += magnitude;
                        count += 1.0;
                    }
                }
            }
            grad[i][j] = sum / count;
        }
    }

    grad
}

pub fn compute_local_max(v: &Vec<Vec<f32>>, kernel_size: isize) -> Vec<Vec<f32>> {
    let mut max: Vec<Vec<f32>> = vec![vec![0.0; v[0].len()]; v.len()];
    let mut max_check: Vec<Vec<f32>> = vec![vec![0.0; v[0].len()]; v.len()];
    let k = kernel_size / 2;

    for i in (k as usize)..v.len()-(k as usize) {
        for j in (k as usize)..v[i].len()-(k as usize) {
            let mut temp = 0.0;
            for m in -k..=k {
                for n in -k..=k {
                    let i_index = i as isize + m;
                    let j_index = j as isize + n;

                    let val = v[i_index as usize][j_index as usize];
                    if val > temp {
                        temp = val;
                    }
                    
                }
            }
            max[i][j] = temp;
        }
    }

    for i in 0..v.len() {
        for j in 0..v[i].len() {
            if max[i][j] == v[i][j] && v[i][j] != 0.0 {
                max_check[i][j] = 254.0;
            }
            else {
                max_check[i][j] = 1.0;
            }
        }
    }

    max_check
}

pub fn compute_gradient_variance(grad: &Vec<Vec<f32>>, kernel_size: isize) -> Vec<Vec<f32>> {
    let mut var: Vec<Vec<f32>> = vec![vec![0.0; grad[0].len()]; grad.len()];
    let k = kernel_size / 2;

    for i in (k as usize)..grad.len()-(k as usize) {
        for j in (k as usize)..grad[i].len()-(k as usize) {
            let mut sum = 0.0;
            let mut sum_sq = 0.0;
            let mut count = 0.0;
            for m in -k..=k {
                for n in -k..=k {
                    let i_index = i as isize + m;
                    let j_index = j as isize + n;
                    if i_index-k >= 0 && i_index+k < grad.len() as isize && j_index-k >= 0 && j_index+k < grad[i].len() as isize {
                        let g = grad[i_index as usize][j_index as usize];
                        sum += g;
                        sum_sq += g * g;
                        count += 1.0;
                    }
                }
            }
            let mean = sum / count;
            let variance = sum_sq / count - mean * mean;
            var[i][j] = variance;
        }
    }

    var
}


fn max_gradient(gradient: Vec<Vec<(f32, f32)>>) -> Vec<Vec<f32>> {
    gradient.iter()
        .map(|row| {
            row.iter()
                .map(|&(x, y)| if x.abs() > y.abs() { x } else { y })
                .collect()
        })
        .collect()
}

fn main() -> Result<(), Box<dyn Error>> {
    let csv_path = "assets/filtered_buildings_tiles_groundFix.csv";
    let las_path = "assets/32_684000_4930000.las"; 

    let df = CsvReader::from_path(csv_path).unwrap()
        .has_header(true)
        .with_separator(b';')
        .finish()
        .unwrap();

    let mut df_f = df.select(&["CODICE_OGGETTO", "QUOTA_PIEDE", "QUOTA_GRONDA", "Geo Shape UTM", "Geo Shape UTM_OFF", "Toll_Tetto"]).unwrap();

    //println!("{:?}", df_f);

    // Complex roof with many segments(T main): clustering pretty good
    // let c_ogg = 24069;

    // Easier roof (L shape): clustering perfect
    let c_ogg = 52578;

    // Flat roof with inner roof access roof element: clustering bad
    // let c_ogg = 45617;

    // Square roof with many objects on top: clustering ok
    // let c_ogg: i64 = 37431;

    // Flat roof with many branches on top: clustering ok
    // let c_ogg = 45415;

    let mask: BooleanChunked = df_f.column("CODICE_OGGETTO")?.i64()?.into_iter().map(|e| e.unwrap() == c_ogg).collect();
    let entry = df_f.filter(&mask).unwrap();

    // println!("{:?}", entry);

    let poly = get_polygons(entry.clone(), true).unwrap().into_iter().nth(0).unwrap();
    let q_gr = entry.column("QUOTA_GRONDA").unwrap().get(0).unwrap().try_extract::<f64>().unwrap();
    let q_tol = entry.column("Toll_Tetto").unwrap().get(0).unwrap().try_extract::<f64>().unwrap();

    let mut reader = LasReader::new(las_path).unwrap();

    let points = reader.get_points().unwrap();

    let mut f_points: Vec<las::Point> = Vec::new();
    let mut f_p32: Vec<myPoint> = Vec::new();

    for point in points {
        let mut point = point.unwrap();

        if poly.contains(&geo::Point::new(point.x, point.y)) {
            if q_gr <= point.z && point.z <= (q_gr + q_tol) {
                f_points.push(point.clone());
                f_p32.push(myPoint::new(point.x as f32, point.y as f32, point.z as f32));
            }
        }
    }

    let verts: Vec<(f32, f32)> = poly.exterior().points().map(|p| (p.x() as f32, p.y() as f32)).collect();

    let mut clusters = dbscan(f_p32, 1., 10);
    merge_clusters(&mut clusters, 0.5);

    println!("Num Clusters: {:?}", clusters.len());

    draw_clusters(verts.clone(), "clusters", RED, &clusters, 1.);

    let largest_cluster = clusters.into_iter().max_by_key(|c| c.len()).unwrap();

    draw_height_cloud(verts.clone(), "heightMap", RED, &largest_cluster, 1.);

    let mut grid = Grid::new(&largest_cluster, 0.1, 2., 0.2);

    draw_height_cloud_grid("heightMapGrid", &grid, 1.);

    let mut mat: Vec<Vec<f32>> = grid.get_height_mat().clone();

    let n = 3;

    for _ in 0..n {
        fill_holes(&mut mat, 4);
    }

    let mut max = compute_local_max(&mat, 11);
    
    create_height_rgb_image("testRgb", &mat);
    create_height_grayscale_image("testGray", &mat);

    create_height_grayscale_image("testGrayMax", &max);

    //let mut gradient = compute_gradient(&mat, 7);

    // let mut max_grad  = compute_average_gradient(&mat, 7);
    let mut max_grad  = compute_average_gradient(&mat, 3);

    create_gradient_grayscale_image("gradient", &max_grad);
    create_height_rgb_image("rgbGradient", &max_grad);

    let mut variance = compute_gradient_variance(&max_grad, 3);

    create_gradient_grayscale_image("variance", &variance);

    // println!("{:?}", max_grad);

    let mut tol: f32 = 8.0;

    for row in &mut max_grad {
        for val in row.iter_mut() {
            if val.abs() > tol {
                *val = 1.0;
            }
            else {
                *val = 0.0;
            }
        }
    }

    create_gradient_grayscale_image("boolGradient", &max_grad);

    let mut tol: f32 = 0.001;

    for row in &mut variance {
        for val in row.iter_mut() {
            if val.abs() > tol {
                *val = 1.0;
            }
            else {
                *val = 0.0;
            }
        }
    }

    create_gradient_grayscale_image("boolVar", &variance);

    // ###############################################################
    // _____________________ Img Processing __________________________
    // ###############################################################

    let img = image::open("output/testGray.png").unwrap().to_luma8();

    // 3.5 good
    let img_blur = imageproc::filter::gaussian_blur_f32(&img, 5.0);

    img_blur.save("output/blur.png");

    let edges = canny(&img_blur, 50.0, 100.0);

    edges.save("output/canny.png");

    let cl_edg = close(&edges, Norm::L1, 5);

    cl_edg.save("output/canny_close.png");

    let target = edges.clone();

    let corners = corners_fast12(&target, 20);

    println!("Corn len: {:?}", corners.len());

    let max_score = corners.iter().map(|c| c.score).fold(0./0., f32::max);
    let sum: f32 = corners.iter().map(|c| c.score).sum();
    let avg_score = sum / (corners.len() as f32);

    println!("Avg Score: {:?}", avg_score);
    println!("Max Score: {:?}", max_score);

    let mut edg_r: RgbImage = ImageBuffer::from_fn(target.width(), target.height(), |x, y| {
        let pixel = target.get_pixel(x, y);
        if pixel[0] == 255 {
            Rgb([255,0,0])
        }
        else {
            Rgb([0,0,0])
        }
    });

    let mut edg_b: ImageBuffer<Luma<u8>, Vec<u8>> = ImageBuffer::new(target.width(), target.height());

    for corn in corners.iter() {
        let norm_sc = corn.score / max_score;
        let i = (norm_sc * 255.0) as u8;
        edg_r.put_pixel(corn.x, corn.y, Rgb([i,i,i]));
        edg_b.put_pixel(corn.x, corn.y, Luma([i]));
    }

    edg_r.save("output/corners_bgr.png");
    edg_b.save("output/corners.png");

    /// #### Hough Transf ####
    
    let options = LineDetectionOptions {
        vote_threshold: 40,
        suppression_radius: 10,
    };

    let lines: Vec<PolarLine> = detect_lines(&target, options);

    println!("lines = {:?}", lines.len());

    let mut img_lines = target.clone();

    draw_polar_lines_mut(&mut img_lines, &lines, Luma([100]));

    img_lines.save("output/lines.png");


    /*
    // ######### Tentativi Downsampling #########

    let (w, h) = cl_edg.dimensions();
    let factor = 4;
    let new_w = w / factor;
    let new_h = h / factor;

    let small_edg = imageops::resize(&cl_edg, new_w, new_h, imageops::FilterType::Nearest);

    let sc_edg = close(&small_edg, Norm::L1, 5);

    let s_corners = corners_fast12(&sc_edg, 20);

    println!("Corn len: {:?}", s_corners.len());

    let max_score = s_corners.iter().map(|c| c.score).fold(0./0., f32::max);
    let sum: f32 = s_corners.iter().map(|c| c.score).sum();
    let avg_score = sum / (s_corners.len() as f32);

    println!("Avg Score: {:?}", avg_score);
    println!("Max Score: {:?}", max_score);

    let mut small_edg_r: RgbImage = ImageBuffer::from_fn(sc_edg.width(), sc_edg.height(), |x, y| {
        let pixel = sc_edg.get_pixel(x, y);
        if pixel[0] == 255 {
            Rgb([255,0,0])
        }
        else {
            Rgb([0,0,0])
        }
    });

    for corn in s_corners.iter() {
        let norm_sc = corn.score / max_score;
        let i = (norm_sc * 255.0) as u8;
        small_edg_r.put_pixel(corn.x, corn.y, Rgb([i,i,i]));
    }

    small_edg_r.save("output/temp/near_corners.png");

    */
    
    /*
    let mut lines = hough_transform(&max_grad, 1.2);

    println!("Num lines: {:?}", lines.len());

    create_gradient_greyscale_image_l("gradient_lines", &max_grad, &lines);
    */

    Ok(())
}
