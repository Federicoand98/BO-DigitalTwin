use crate::my_point::*;

pub fn dbscan(points: Vec<myPoint>, eps: f32, min_points: usize) -> Vec<Vec<myPoint>> {
    let mut clusters = Vec::new();
    let mut visited = vec![false; points.len()];

    for (i, point) in points.iter().enumerate() {
        if visited[i] {
            continue;
        }
        visited[i] = true;

        let neighb = find_neighbors(&points, point, eps);

        if neighb.len() < min_points {
            continue;
        }

        let mut cluster = vec![point.clone()];

        for n in neighb {
            if !visited[n] {
                visited[n] = true;
                let near_n = find_neighbors(&points, &points[n], eps);
                if near_n.len() >= min_points {
                    cluster.push(points[n].clone());
                }
            }
        }

        clusters.push(cluster);
    }

    clusters
}

pub fn merge_clusters(clusters: &mut Vec<Vec<myPoint>>, eps: f32) {
    let mut merged = true;
    while merged {
        merged = false;
        for i in 0..clusters.len() {
            for j in i+1..clusters.len() {
                if clusters[i].iter().any(|p1| clusters[j].iter().any(|p2| dist(p1, p2) <= eps)) {
                    let mut cluster_j = clusters.remove(j);
                    clusters[i].append(&mut cluster_j);
                    merged = true;
                    break;
                }
            }
            if merged {
                break;
            }
        }
    }
}

fn find_neighbors(points: &Vec<myPoint>, point: &myPoint, eps: f32) -> Vec<usize> {
    points.iter().enumerate()
        .filter(|(_, p)| dist(p, point) <= eps)
        .map(|(i, _)| i)
        .collect()
}

fn dist(p1: &myPoint, p2: &myPoint) -> f32 {
    ((p1.x - p2.x).powi(2) + (p1.y - p2.y).powi(2) + (p1.z - p2.z).powi(2)).sqrt()
}