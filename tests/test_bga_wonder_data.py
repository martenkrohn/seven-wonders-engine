"""Independent frozen target comparison with actual compiled wonder classes."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class WonderDataTests(unittest.TestCase):
    def test_compiled_wonders_match_targets(self):
        binary=Path(os.environ.get('SEVENWONDERS_WONDER_TEST',ROOT/'build/test_wonder_tables')).resolve()
        with tempfile.TemporaryDirectory() as tmp:
            result=subprocess.run([str(binary),'--dump'],cwd=tmp,capture_output=True,text=True,timeout=10)
        self.assertEqual(result.returncode,0,result.stderr)
        actual=json.loads(result.stdout)
        self.assertEqual({r['engine_id'] for r in actual},{0,1,3,4,5,7,8,10,11,12})
        targets={r['engine_id']:r for r in json.loads((ROOT/'data/bga-second-edition/targets.json').read_text())['wonders']}
        for row in actual:
            self.assertEqual(row,{k:targets[row['engine_id']][k] for k in row})
