using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

namespace Klak.Hap
{
    internal sealed class StreamReader : IDisposable
    {
        #region Public methods

        public StreamReader(Demuxer demuxer)
        {
            _demuxer = demuxer;

            _leadQueue = new Queue<ReadBuffer>();
            _freeQueue = new Queue<ReadBuffer>();

            // Initial queue entry allocation
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());
            _freeQueue.Enqueue(new ReadBuffer());

            // By default, read from the first frame with x1 speed.
            _time = 0;
            _delta = DefaultDelta;

            // Reader thread startup
            _resume = new AutoResetEvent(true);
            _thread = new Thread(ReaderThread);
            _thread.Start();
        }

        public void Dispose()
        {
            if (_thread != null)
            {
                _time = -1;
                _resume.Set();
                _thread.Join();
                _thread = null;
            }

            if (_resume != null)
            {
                _resume.Dispose();
                _resume = null;
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

        public void Reschedule(float time, float delta)
        {
            // Flush out the current contents of the lead queue
            lock (_leadQueue)
                lock (_freeQueue)
                    while (_leadQueue.Count > 0)
                        _freeQueue.Enqueue(_leadQueue.Dequeue());

            // Update the time configurations.
            _time = time;
            _delta = delta;

            // The delta value should be larger than the single frame duration.
            if (Math.Abs(_delta) < DefaultDelta)
                _delta = DefaultDelta * Math.Sign(_delta);

            // Notify the changes to the reader thread.
            _resume.Set();
        }

        public ReadBuffer Advance(float time)
        {
            // Free the current frame.
            if (_current != null)
            {
                lock (_freeQueue) _freeQueue.Enqueue(_current);
                _current = null;
            }

            // Scan the lead queue and free old frames.
            lock (_leadQueue) while (_leadQueue.Count > 0 && _leadQueue.Peek().Time < time)
            {
                if (_current != null) lock (_freeQueue) _freeQueue.Enqueue(_current);
                _current = _leadQueue.Dequeue();
            }

            // Notify the changes to the reader thread.
            _resume.Set();

            return _current;
        }

        #endregion

        #region Private members

        Thread _thread;
        AutoResetEvent _resume;

        Demuxer _demuxer;
        ReadBuffer _current;
        Queue<ReadBuffer> _leadQueue;
        Queue<ReadBuffer> _freeQueue;

        float _time, _delta;

        float DefaultDelta { get {
            return (float)(_demuxer.Duration / _demuxer.FrameCount);
        } }

        #endregion

        #region Thread function

        void ReaderThread()
        {
            while (true)
            {
                _resume.WaitOne();

                if (_time < 0) break;

                lock (_freeQueue)
                {
                    if (_freeQueue.Count == 0) continue;

                    var buffer = _freeQueue.Dequeue();
                    _demuxer.ReadFrameAtTime(_time, buffer);

                    lock (_leadQueue) _leadQueue.Enqueue(buffer);
                }

                _time += _delta;
            }
        }

        #endregion
    }
}
