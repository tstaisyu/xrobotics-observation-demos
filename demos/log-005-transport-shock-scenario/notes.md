# Log 005: Transport Shock Scenario

## Purpose
This log uses the implementation from Log 004 to test how adjustable shock-detection rules can be shown in a transport or installation scenario.

## Implementation
This log does not contain separate firmware or app code.
It reuses the implementation from:

../log-004-adjustable-rule/

## Scenario
- Monitor shocks during transport or installation
- Set a strict rule for fragile equipment, such as alert after 2 shocks
- Change the rule for more tolerant conditions, such as alert after 5 shocks
- Show that the same monitoring system can be adjusted to different field conditions

## Video message
- 輸送・設置中の衝撃を監視
- 精密機器想定：2回でアラート
- 条件変更：5回まで許容
- 5回目でアラート
- 現場ごとに異常条件を調整

## Related code
See: ../log-004-adjustable-rule/
