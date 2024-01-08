use crate::my_point::*;

pub struct Grid {
    min_x: f32,
    max_x: f32,
    min_y: f32,
    max_y: f32,
    cell_size: f32,
    points: Vec<myPoint>,
    height_mat: Vec<Vec<f32>>,
}

impl Grid {
    pub fn new(i_points: &[myPoint], cell_size: f32, tol: f32, radius: f32) -> Self {
        let min_x = i_points.iter().map(|p| p.x).fold(f32::INFINITY, f32::min) - tol;
        let max_x = i_points.iter().map(|p| p.x).fold(f32::NEG_INFINITY, f32::max) + tol;
        let min_y = i_points.iter().map(|p| p.y).fold(f32::INFINITY, f32::min) - tol;
        let max_y = i_points.iter().map(|p| p.y).fold(f32::NEG_INFINITY, f32::max) + tol;

        let num_cells_x = ((max_x - min_x) / cell_size).ceil() as usize;
        let num_cells_y = ((max_y - min_y) / cell_size).ceil() as usize;

        let mut points: Vec<myPoint> = Vec::new();
        let mut height_mat: Vec<Vec<f32>> = vec![vec![0.0; num_cells_y]; num_cells_x];

        for i in 0..num_cells_x {
            for j in 0..num_cells_y {
                let x = min_x + i as f32 * cell_size;
                let y = min_y + j as f32 * cell_size;
                let mut z = 0.0;

                let mut min_distance = radius;
                let mut closest_z = 0.0;
                for point in i_points {
                    let distance = point.distance_xy(&myPoint::new(x, y, 0.0));
                    if distance < min_distance {
                        min_distance = distance;
                        closest_z = point.z;
                        if distance <= 0.02 {
                            break;
                        }
                    }
                }
                z = closest_z;

                points.push(myPoint::new(x, y, z));
                height_mat[i][j] = z;
            }
        }

        Self {
            min_x,
            max_x,
            min_y,
            max_y,
            cell_size,
            points,
            height_mat
        }
    }

    pub fn get_min_x(&self) -> f32 {
        self.min_x
    }

    pub fn get_max_x(&self) -> f32 {
        self.max_x
    }

    pub fn get_min_y(&self) -> f32 {
        self.min_y
    }

    pub fn get_max_y(&self) -> f32 {
        self.max_y
    }

    pub fn get_cell_size(&self) -> f32 {
        self.cell_size
    }

    pub fn get_points(&self) -> &Vec<myPoint> {
        &self.points
    }

    pub fn get_height_mat(&self) -> &Vec<Vec<f32>> {
        &self.height_mat
    }
}