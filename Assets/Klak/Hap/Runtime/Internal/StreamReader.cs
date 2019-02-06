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
            _freeBuffers = new List<ReadBuffer>();

            // Initial buffer entry allocation
            _freeBuffers.Add(new ReadBuffer());
            _freeBuffers.Add(new ReadBuffer());
            _freeBuffers.Add(new ReadBuffer());
            _freeBuffers.Add(new ReadBuffer());

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

            if (_freeBuffers != null)
            {
                foreach (var rb in _freeBuffers) rb.Dispose();
                _freeBuffers.Clear();
                _freeBuffers = null;
            }
        }

        public void Restart(float time, float delta)
        {
            // Restart request
            lock (_restartLock) _restart = (time, SafeDelta(delta));

            // Wait for reset/read on the reader thread.
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
                // Scan the lead queue.
                while (_leadQueue.Count > 0)
                {
                    var peek = _leadQueue.Peek();

                    if (_current != null)
                    {
                        if (_current.Time <= peek.Time)
                        {
                            // Forward playback case:
                            // Break if it hasn't reached the next frame.
                            if (time < peek.Time) break;
                        }
                        else
                        {
                            // Reverse playback case:
                            // Break if it's still on the current frame.
                            if (_current.Time < time) break;
                        }

                        // Free the current frame before replacing it.
                        _freeBuffers.Add(_current);
                    }

                    _current = _leadQueue.Dequeue();
                    changed = true;
                }
            }

            // Poke the reader thread.
            _updateEvent.Set();

            // Only returns a buffer object when the frame was changed.
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

        // Read buffer objects
        ReadBuffer _current;
        Queue<ReadBuffer> _leadQueue;
        List<ReadBuffer> _freeBuffers;
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
            // Initial time settings from the restart request tuple
            var (time, delta) = _restart.Value;
            _restart = null;

            // Stream attributes
            var totalTime = _demuxer.Duration;
            var totalFrames = _demuxer.FrameCount;

            while (true)
            {
                // Synchronization with the parent thread
                _updateEvent.WaitOne();
                if (_terminate) break;

                // Check if there is a restart request.
                lock (_restartLock) if (_restart != null)
                {
                    // Flush out the current contents of the lead queue.
                    lock (_queueLock) while (_leadQueue.Count > 0)
                        _freeBuffers.Add(_leadQueue.Dequeue());

                    // Apply the restart request.
                    (time, delta) = _restart.Value;
                    _restart = null;
                }

                // Time -> Frame count
                var frameCount = Math.Floor(time * totalFrames / totalTime);

                // Frame count -> Frame snapped time
                var snappedTime = (float)(frameCount * totalTime / totalFrames);

                // Frame count -> Wrapped frame number
                var frameNumber = (int)frameCount % totalFrames;
                if (frameNumber < 0) frameNumber += totalFrames;

                lock (_queueLock)
                {
                    // Do nothing if there is no free buffer; It indicates that
                    // the lead queue is fully filled.
                    if (_freeBuffers.Count == 0) continue;

                    ReadBuffer buffer = null;

                    // Look for a free buffer that has the same frame number.
                    foreach (var temp in _freeBuffers)
                    {
                        if (temp.Index == frameNumber)
                        {
                            buffer = temp;
                            break;
                        }
                    }

                    if (buffer != null)
                    {
                        // Reuse the found buffer; Although we can use it
                        // without reading frame data, the time field should
                        // be updated to handle wrapping-around hits.
                        _freeBuffers.Remove(buffer);
                        buffer.Time = snappedTime;
                    }
                    else
                    {
                        // Allocate a buffer from the free buffer list.
                        buffer = _freeBuffers[_freeBuffers.Count - 1];
                        _freeBuffers.RemoveAt(_freeBuffers.Count - 1);

                        // Frame data read
                        _demuxer.ReadFrame(buffer, frameNumber, snappedTime);
                    }

                    // Push the buffer to the lead queue.
                    _leadQueue.Enqueue(buffer);
                }

                _readEvent.Set();

                time += delta;
            }
        }

        #endregion
    }
}
