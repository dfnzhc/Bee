use crate::WorkId;
use rayon::{Scope, ThreadPool};
use std::ops::Range;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Mutex, OnceLock};

#[derive(Clone, Debug)]
pub(crate) struct CpuSchedule {
    predecessors: Vec<usize>,
    successors: Vec<Vec<WorkId>>,
}

impl CpuSchedule {
    pub fn new(edges: &[Vec<bool>]) -> Self {
        let mut predecessors = vec![0; edges.len()];
        let successors = edges
            .iter()
            .map(|row| {
                row.iter()
                    .enumerate()
                    .filter_map(|(index, &edge)| {
                        edge.then(|| {
                            predecessors[index] += 1;
                            WorkId(index)
                        })
                    })
                    .collect()
            })
            .collect();
        Self {
            predecessors,
            successors,
        }
    }

    pub fn add_predecessors(&mut self, work: WorkId, count: usize) -> Range<usize> {
        let start = self.predecessors.len();
        self.predecessors[work.0] += count;
        self.predecessors.resize(start + count, 0);
        self.successors.extend((0..count).map(|_| vec![work]));
        start..start + count
    }

    pub fn run<T: Send + Sync>(
        &self,
        pool: &ThreadPool,
        order: &[WorkId],
        run: impl Fn(WorkId, &[OnceLock<T>]) -> Result<T, String> + Sync,
    ) -> Result<Vec<T>, String> {
        let batch = Batch {
            remaining: self
                .predecessors
                .iter()
                .copied()
                .map(AtomicUsize::new)
                .collect(),
            results: self.predecessors.iter().map(|_| OnceLock::new()).collect(),
            first_error: Mutex::new(None),
            successors: &self.successors,
            run,
        };
        pool.in_place_scope(|scope| {
            for (index, &count) in self.predecessors.iter().enumerate().rev() {
                if count == 0 {
                    batch.spawn(scope, WorkId(index));
                }
            }
        });
        if let Some(error) = batch.first_error.into_inner().unwrap() {
            return Err(error);
        }
        let mut results = batch.results;
        Ok(order
            .iter()
            .map(|work| results[work.0].take().expect("unscheduled CPU work"))
            .collect())
    }
}

struct Batch<'a, T, F> {
    remaining: Vec<AtomicUsize>,
    results: Vec<OnceLock<T>>,
    first_error: Mutex<Option<String>>,
    successors: &'a [Vec<WorkId>],
    run: F,
}

impl<T: Send + Sync, F: Fn(WorkId, &[OnceLock<T>]) -> Result<T, String> + Sync> Batch<'_, T, F> {
    fn spawn<'scope>(&'scope self, scope: &Scope<'scope>, work: WorkId) {
        scope.spawn(move |scope| {
            match (self.run)(work, &self.results) {
                Ok(result) => assert!(self.results[work.0].set(result).is_ok()),
                Err(error) => {
                    self.first_error.lock().unwrap().get_or_insert(error);
                    return;
                }
            }
            for &next in &self.successors[work.0] {
                // 最后一个前置完成即派发后继，不等待同批的独立分支。
                if self.remaining[next.0].fetch_sub(1, Ordering::AcqRel) == 1 {
                    self.spawn(scope, next);
                }
            }
        });
    }
}
