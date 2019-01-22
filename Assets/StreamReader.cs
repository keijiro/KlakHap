using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

namespace Klak.Hap
{
    internal sealed class StreamReader : IDisposable
    {
        #region Public methods

        public StreamReader(Demuxer demuxer, float time, float delta)
        {
            _demuxer = demuxer;

            _leadQueue = new Queue<ReadBuffer>();
            _freeQueue = new Queue<ReadBuffer>();

            // Initial queue entry allocation
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());

            // Initial playback settings
            _restart = (time, SafeDelta(delta));

            // Reader thread startup
            _updateEvent = new AutoResetEvent(true);
            _readEvent = new AutoResetEvent(false);
            _thread = new Thread(ReaderThread);
            _thread.Start();
        }

        public void Dispose()
        {
            if (_thread != null)
            {
                _terminate = true;
                _updateEvent.Set();
                _thread.Join();
                _thread = null;
            }

            if (_updateEvent != null)
            {
                _updateEvent.Dispose();
                _updateEvent = null;
            }

            if (_readEvent != null)
            {
                _readEvent.Dispose();
                _readEvent = null;
            }

            if (_current != null)
            {
                _current.Dispose();
                _current = null;
            }

            if (_leadQueue != null)
            {
                foreach (var rb in _leadQueue) rb.Dispose();
                _leadQueue.Clear();
                _leadQueue = null;
            }

            if (_freeQueue != null)
            {
                foreach (var rb in _freeQueue) rb.Dispose();
                _freeQueue.Clear();
                _freeQueue = null;
            }
        }

        public void Restart(float time, float delta)
        {
            // Restart request
            lock (_restartLock) _restart = (time, SafeDelta(delta));

            // Wait for reset and read on the reader thread.
            _readEvent.Reset();
            while (_restart != null)
            {
                _updateEvent.Set();
                _readEvent.WaitOne();
            }
        }

        public ReadBuffer Advance(float time)
        {
            var changed = false;

            // There is no slow path in this function, so we prefer holding
            // the queue lock for the entire function block rather than
            // acquiring/releasing it for each operation.
            lock (_queueLock)
            {
                // Scan the lead queue and free old frames.
                while (_leadQueue.Count > 0)
                {
                    var peek = _leadQueue.Peek();

                    if (_current != null)
                    {
                        // Break if the current frame is closer than the
                        // peeked one.
                        var dif_c = Math.Abs(_current.Time - time);
                        var dif_n = Math.Abs(peek.Time - time);

                        if (dif_c < dif_n) break;

                        // Free the current frame before replacing it.
                        _freeQueue.Enqueue(_current);
                    }

                    _current = _leadQueue.Dequeue();
                    changed = true;
                }
            }

            // Notify the changes to the reader thread.
            if (changed) _updateEvent.Set();

            return changed ? _current : null;
        }

        #endregion

        #region Private members

        // Assigned demuxer
        Demuxer _demuxer;

        // Thread and synchronization objects
        Thread _thread;
        AutoResetEvent _updateEvent;
        AutoResetEvent _readEvent;
        bool _terminate;

        // Read buffer and queue objects
        ReadBuffer _current;
        Queue<ReadBuffer> _leadQueue;
        Queue<ReadBuffer> _freeQueue;
        readonly object _queueLock = new object();

        // Restart request
        (float, float)? _restart;
        readonly object _restartLock = new object();

        // Used to avoid too small delta time values.
        float SafeDelta(float delta)
        {
            var min = (float)(_demuxer.Duration / _demuxer.FrameCount);
            return Math.Max(Math.Abs(delta), min) * (delta < 0 ? -1 : 1);
        }

        #endregion

        #region Thread function

        void ReaderThread()
        {
            // Get initial time settings from the restart request tuple.
            var (time, delta) = _restart.Value;
            _restart = null;

            // Total length of the stream
            var total = (float)_demuxer.Duration;

            while (true)
            {
                _updateEvent.WaitOne();
                if (_terminate) break;

                // Check if there is a restart request.
                lock (_restartLock) if (_restart != null)
                {
                    // Flush out the current contents of the lead queue.
                    lock (_queueLock)
                        while (_leadQueue.Count > 0)
                            _freeQueue.Enqueue(_leadQueue.Dequeue());

                    // Apply the restart settings.
                    (time, delta) = _restart.Value;
                    _restart = null;
                }

                // Allocate a read buffer from freeQueue.
                ReadBuffer buffer;
                lock (_queueLock)
                {
                    if (_freeQueue.Count == 0) continue;
                    buffer = _freeQueue.Dequeue();
                }

                // Time wrapping
                var wrappedTime = time % total;
                if (wrappedTime < 0) wrappedTime += total;

                // Frame data read
                _demuxer.ReadFrameAtTime(wrappedTime, buffer);

                // Replace the time field with the unwrapped time value.
                buffer.Time = time;

                // Push the read data to the leadQueue.
                lock (_queueLock) _leadQueue.Enqueue(buffer);
                _readEvent.Set();

                time += delta;
            }
        }

        #endregion
    }
}
