use std::sync::{
    atomic::{AtomicU64, Ordering},
    mpsc::{sync_channel, Receiver, SyncSender},
    Arc,
};

pub const DIAGNOSTIC_QUEUE_CAPACITY: usize = 256;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DiagnosticLevel {
    Info,
    Warning,
    Error,
}

#[derive(Debug)]
pub struct DiagnosticMessage {
    pub level: DiagnosticLevel,
    pub source: &'static str,
    pub message: String,
}

#[derive(Clone)]
pub struct DiagnosticSender {
    sender: SyncSender<DiagnosticMessage>,
    dropped: Arc<AtomicU64>,
}

pub struct DiagnosticReceiver {
    receiver: Receiver<DiagnosticMessage>,
    dropped: Arc<AtomicU64>,
}

pub fn diagnostic_channel() -> (DiagnosticSender, DiagnosticReceiver) {
    let (sender, receiver) = sync_channel(DIAGNOSTIC_QUEUE_CAPACITY);
    let dropped = Arc::new(AtomicU64::new(0));
    (
        DiagnosticSender {
            sender,
            dropped: dropped.clone(),
        },
        DiagnosticReceiver { receiver, dropped },
    )
}

impl DiagnosticSender {
    pub fn send(&self, level: DiagnosticLevel, source: &'static str, message: impl Into<String>) {
        // 回调线程只尝试投递；队列满或接收端关闭时不等待 UI。
        if self
            .sender
            .try_send(DiagnosticMessage {
                level,
                source,
                message: message.into(),
            })
            .is_err()
        {
            self.dropped.fetch_add(1, Ordering::Relaxed);
        }
    }
}

impl DiagnosticReceiver {
    pub fn drain(&self) -> impl Iterator<Item = DiagnosticMessage> + '_ {
        // 单次处理有上限，持续到来的诊断不能占满 UI 线程。
        self.receiver.try_iter().take(DIAGNOSTIC_QUEUE_CAPACITY)
    }

    pub fn take_dropped(&self) -> u64 {
        self.dropped.swap(0, Ordering::Relaxed)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn bounded_queue_counts_overflow_and_sender_survives_receiver_shutdown() {
        let (sender, receiver) = diagnostic_channel();
        std::thread::scope(|scope| {
            for _ in 0..4 {
                let sender = sender.clone();
                scope.spawn(move || {
                    for _ in 0..DIAGNOSTIC_QUEUE_CAPACITY {
                        sender.send(DiagnosticLevel::Warning, "test", "background message");
                    }
                });
            }
        });
        assert_eq!(receiver.drain().count(), DIAGNOSTIC_QUEUE_CAPACITY);
        assert_eq!(
            receiver.take_dropped(),
            3 * DIAGNOSTIC_QUEUE_CAPACITY as u64
        );
        assert_eq!(receiver.take_dropped(), 0);
        drop(receiver);
        sender.send(DiagnosticLevel::Error, "test", "after shutdown");
    }
}
