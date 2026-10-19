# 에디터 시작 시 자동 실행 (PythonScriptPlugin이 Content/Python/init_unreal.py를 실행한다)
#
# MCP EditorToolset(5.8 실험 기능) 패치:
# BlueprintTools의 노드 타입 캐시가 graph.get_outer()를 바로 Blueprint로 cast 하는데,
# 애님 BP의 상태/전환 규칙 그래프는 outer가 AnimStateNode/AnimStateTransitionNode라서
# cast가 예외를 던진다 → find_node_types/create_node가 중첩 그래프에서 실패.
# outer 체인을 따라 올라가 Blueprint를 찾도록 두 메서드만 교체한다.
# shortcut: 엔진 내부 클래스(_BlueprintCache)에 의존, 엔진 업데이트 시 이 패치가 아직 필요한지 확인

import unreal

try:
    from editor_toolset.toolsets import blueprint as _bp_tools
except ImportError:
    _bp_tools = None


def _owning_blueprint(graph):
    outer = graph.get_outer()
    while outer is not None and not isinstance(outer, unreal.Blueprint):
        outer = outer.get_outer()
    return outer


def _fingerprint(graph):
    blueprint = _owning_blueprint(graph)
    if not blueprint:
        return ''
    var_names = sorted(
        str(v) for v in unreal.BlueprintEditorLibrary.list_member_variable_names(blueprint, False))
    graph_names = sorted(g.get_name() for g in unreal.BlueprintEditorLibrary.list_graphs(blueprint))
    return f'{var_names}|{graph_names}'


def _get(cls, graph):
    graph_key = graph.get_path_name()
    fingerprint = cls._fingerprint(graph)
    entry = cls._graph_node_types.get(graph_key)
    if entry is not None:
        if entry.fingerprint == fingerprint:
            return entry.type_ids_by_category
        blueprint = _owning_blueprint(graph)
        if blueprint:
            cls.invalidate(blueprint)
    entry = _bp_tools._CacheEntry(fingerprint=fingerprint)
    for node in cls.list_nodes(graph):
        entry.type_ids_by_category['|'.join(node.split('|')[:-1])].append(node)
    blueprint = _owning_blueprint(graph)
    if blueprint:
        for name in unreal.BlueprintEditorLibrary.list_event_dispatchers(blueprint):
            entry.type_ids_by_category['Default'].append(f'{_bp_tools._DISPATCHER_PREFIX}{name}')
    cls._graph_node_types[graph_key] = entry
    return entry.type_ids_by_category


if _bp_tools is not None:
    _bp_tools._BlueprintCache._fingerprint = staticmethod(_fingerprint)
    _bp_tools._BlueprintCache.get = classmethod(_get)
    unreal.log('[Hunt] EditorToolset BlueprintTools nested-graph patch applied')
