use super::{ACCENT, HOVER, MUTED, PANEL, SELECTED, TEXT};
use crate::Bee;
use bee_scene::SceneState;
use gpui::{div, prelude::*, px, rgb, App, Div, Entity, KeyDownEvent, SharedString};
use gpui_component::{Icon, IconName, Sizable, StyledExt};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(super) enum Item {
    Instance(usize),
    Light(usize),
    Camera,
}

impl Item {
    fn exists(self, state: &SceneState) -> bool {
        match self {
            Self::Instance(index) => index < state.scene().instances.len(),
            Self::Light(index) => index < state.scene().lights.len(),
            Self::Camera => true,
        }
    }

    fn label(self) -> String {
        match self {
            Self::Instance(index) => format!("Instance {}", index + 1),
            Self::Light(index) => format!("Point light {}", index + 1),
            Self::Camera => "Camera".into(),
        }
    }

    fn selector(self) -> String {
        match self {
            Self::Instance(index) => format!("scene-instance-{index}"),
            Self::Light(index) => format!("scene-light-{index}"),
            Self::Camera => "scene-camera".into(),
        }
    }

    fn icon(self) -> IconName {
        match self {
            Self::Instance(_) => IconName::Frame,
            Self::Light(_) => IconName::Sun,
            Self::Camera => IconName::Eye,
        }
    }
}

#[derive(Default)]
pub(super) struct Selection(Option<(Item, u64)>);

impl Selection {
    pub(super) fn current(&self, state: &SceneState) -> Option<Item> {
        self.0.and_then(|(item, version)| {
            (version == state.versions().scene && item.exists(state)).then_some(item)
        })
    }

    pub(super) fn is_stale(&self, state: &SceneState) -> bool {
        self.0.is_some() && self.current(state).is_none()
    }

    pub(super) fn clear(&mut self) {
        self.0 = None;
    }
}

fn select(bee: &Entity<Bee>, selection: &Entity<Selection>, item: Option<Item>, cx: &mut App) {
    let state = &bee.read(cx).state;
    if item.is_some_and(|item| !item.exists(state)) {
        return;
    }
    let next = item.map(|item| (item, state.versions().scene));
    selection.update(cx, |selection, cx| {
        if selection.0 != next {
            selection.0 = next;
            cx.notify();
        }
    });
}

fn heading(name: &'static str, count: usize) -> Div {
    div()
        .flex()
        .justify_between()
        .items_center()
        .pt_1()
        .pb_0p5()
        .px_2()
        .text_size(px(11.0))
        .text_color(rgb(MUTED))
        .child(name)
        .child(count.to_string())
}

fn item_row(
    item: Item,
    selected: bool,
    bee: Entity<Bee>,
    selection: Entity<Selection>,
) -> impl IntoElement {
    let selector = item.selector();
    let key_bee = bee.clone();
    let key_selection = selection.clone();
    div()
        .id(SharedString::from(selector.clone()))
        .debug_selector(move || selector)
        .tab_index(0)
        .tab_stop(true)
        .flex()
        .items_center()
        .gap_1p5()
        .h(px(24.0))
        .px_2()
        .rounded_sm()
        .border_1()
        .border_color(rgb(if selected { ACCENT } else { PANEL }))
        .bg(rgb(if selected { SELECTED } else { PANEL }))
        .text_color(rgb(TEXT))
        .cursor_pointer()
        .hover(|style| style.bg(rgb(if selected { SELECTED } else { HOVER })))
        .focus(|style| style.border_color(rgb(ACCENT)))
        .child(Icon::new(item.icon()).xsmall())
        .child(div().flex_1().min_w_0().text_ellipsis().child(item.label()))
        .when(selected, |row| {
            row.child(Icon::new(IconName::Check).xsmall())
        })
        .on_click(move |_, _, cx| select(&bee, &selection, Some(item), cx))
        .on_key_down(move |event: &KeyDownEvent, _, cx| {
            if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                select(&key_bee, &key_selection, Some(item), cx);
            }
        })
}

pub(super) fn scene_list(
    state: &SceneState,
    selection: &Selection,
    bee: Entity<Bee>,
    selection_entity: Entity<Selection>,
) -> Div {
    let current = selection.current(state);
    let scene = state.scene();
    let mut list = div().flex().flex_col().tab_group();
    if current.is_some() {
        let clear_bee = bee.clone();
        let clear_selection = selection_entity.clone();
        let key_bee = bee.clone();
        let key_selection = selection_entity.clone();
        list = list.child(
            div()
                .id("scene-clear")
                .debug_selector(|| "scene-clear".into())
                .tab_index(0)
                .tab_stop(true)
                .px_2()
                .py_0p5()
                .rounded_sm()
                .border_1()
                .border_color(rgb(PANEL))
                .text_size(px(11.0))
                .text_color(rgb(MUTED))
                .cursor_pointer()
                .hover(|style| style.bg(rgb(HOVER)))
                .focus(|style| style.border_color(rgb(ACCENT)))
                .child("Clear selection")
                .on_click(move |_, _, cx| select(&clear_bee, &clear_selection, None, cx))
                .on_key_down(move |event: &KeyDownEvent, _, cx| {
                    if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                        select(&key_bee, &key_selection, None, cx);
                    }
                }),
        );
    }
    list = list.child(heading("Instances", scene.instances.len()));
    if scene.instances.is_empty() {
        list = list.child(
            div()
                .debug_selector(|| "scene-instances-empty".into())
                .px_2()
                .text_color(rgb(MUTED))
                .child("No instances"),
        );
    }
    for index in 0..scene.instances.len() {
        let item = Item::Instance(index);
        list = list.child(item_row(
            item,
            current == Some(item),
            bee.clone(),
            selection_entity.clone(),
        ));
    }
    list = list.child(heading("Lights", scene.lights.len()));
    if scene.lights.is_empty() {
        list = list.child(
            div()
                .debug_selector(|| "scene-lights-empty".into())
                .px_2()
                .text_color(rgb(MUTED))
                .child("No lights"),
        );
    }
    for index in 0..scene.lights.len() {
        let item = Item::Light(index);
        list = list.child(item_row(
            item,
            current == Some(item),
            bee.clone(),
            selection_entity.clone(),
        ));
    }
    list.child(heading("Camera", 1)).child(item_row(
        Item::Camera,
        current == Some(Item::Camera),
        bee,
        selection_entity,
    ))
}

fn vector(values: [f32; 3]) -> String {
    format!("{}, {}, {}", values[0], values[1], values[2])
}

fn details(
    state: &SceneState,
    item: Item,
) -> Option<(String, Vec<(&'static str, &'static str, String)>)> {
    let scene = state.scene();
    let fields = match item {
        Item::Instance(index) => {
            let instance = scene.instances.get(index)?;
            let mut fields = vec![
                (
                    "geometry",
                    "Geometry",
                    format!("Index {}", instance.geometry),
                ),
                (
                    "material",
                    "Material",
                    format!("Index {}", instance.material),
                ),
            ];
            let albedo = scene
                .materials
                .get(instance.material)
                .map(|material| vector(material.albedo))
                .unwrap_or_else(|| "Unavailable".into());
            fields.push(("albedo", "Diffuse albedo (RGB)", albedo));
            let matrix = instance.transform.to_array();
            for (index, (key, label)) in [
                ("transform-1", "Transform row 1"),
                ("transform-2", "Transform row 2"),
                ("transform-3", "Transform row 3"),
                ("transform-4", "Transform row 4"),
            ]
            .into_iter()
            .enumerate()
            {
                let row = &matrix[index * 4..index * 4 + 4];
                fields.push((
                    key,
                    label,
                    format!("{}, {}, {}, {}", row[0], row[1], row[2], row[3]),
                ));
            }
            fields
        }
        Item::Light(index) => {
            let light = scene.lights.get(index)?;
            vec![
                (
                    "position",
                    "Position (XYZ)",
                    vector([light.position.x, light.position.y, light.position.z]),
                ),
                ("intensity", "Intensity (RGB)", vector(light.intensity)),
            ]
        }
        Item::Camera => {
            let camera = state.camera();
            let position = camera.position();
            vec![
                (
                    "position",
                    "Position (XYZ)",
                    vector([position.x, position.y, position.z]),
                ),
                (
                    "target",
                    "Target (XYZ)",
                    vector([camera.target.x, camera.target.y, camera.target.z]),
                ),
                ("yaw", "Yaw (degrees)", camera.yaw.to_degrees().to_string()),
                (
                    "pitch",
                    "Pitch (degrees)",
                    camera.pitch.to_degrees().to_string(),
                ),
                ("distance", "Distance", camera.distance.to_string()),
                (
                    "fov",
                    "Vertical FOV (degrees)",
                    camera.vertical_fov.to_degrees().to_string(),
                ),
                ("near", "Near", camera.near.to_string()),
                ("far", "Far", camera.far.to_string()),
            ]
        }
    };
    Some((item.label(), fields))
}

pub(super) fn properties(state: &SceneState, selection: &Selection) -> Div {
    let Some((title, fields)) = selection
        .current(state)
        .and_then(|item| details(state, item))
    else {
        let version = state.versions().scene;
        return div()
            .debug_selector(move || format!("properties-empty-{version}"))
            .text_color(rgb(MUTED))
            .child("No selection");
    };
    div()
        .flex()
        .flex_col()
        .gap_2()
        .child(div().font_semibold().text_color(rgb(TEXT)).child(title))
        .children(fields.into_iter().map(|(key, label, value)| {
            div()
                .debug_selector(move || format!("property-{key}"))
                .flex()
                .flex_col()
                .gap_0p5()
                .child(
                    div()
                        .text_size(px(11.0))
                        .text_color(rgb(MUTED))
                        .child(label),
                )
                .child(div().text_color(rgb(TEXT)).child(value))
        }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::Transform3;

    fn field<'a>(fields: &'a [(&'static str, &'static str, String)], key: &str) -> &'a str {
        &fields.iter().find(|field| field.0 == key).unwrap().2
    }

    #[test]
    fn details_use_the_selected_scene_data() {
        let mut state = SceneState::default();
        let mut scene = state.scene().clone();
        scene.instances[1].transform = Transform3::translation(1.25, -2.5, 3.75);
        let material_index = scene.instances[1].material;
        scene.materials[material_index].albedo = [0.12, 0.34, 0.56];
        scene.lights[0].position = bee_scene::Point3::new(7.0, 8.0, 9.0);
        scene.lights[0].intensity = [10.0, 11.0, 12.0];
        assert!(state.set_scene(scene));

        let (title, fields) = details(&state, Item::Instance(1)).unwrap();
        assert_eq!(title, "Instance 2");
        assert_eq!(
            field(&fields, "material"),
            format!("Index {material_index}")
        );
        assert_eq!(field(&fields, "albedo"), "0.12, 0.34, 0.56");
        assert_eq!(field(&fields, "transform-4"), "1.25, -2.5, 3.75, 1");

        let (_, fields) = details(&state, Item::Light(0)).unwrap();
        assert_eq!(field(&fields, "position"), "7, 8, 9");
        assert_eq!(field(&fields, "intensity"), "10, 11, 12");

        let mut camera = state.camera();
        camera.target = bee_scene::Point3::new(1.0, 2.0, 3.0);
        camera.distance = 8.0;
        assert!(state.set_camera(camera));
        let (_, fields) = details(&state, Item::Camera).unwrap();
        assert_eq!(field(&fields, "target"), "1, 2, 3");
        assert_eq!(field(&fields, "distance"), "8");
        assert_eq!(field(&fields, "near"), camera.near.to_string());
        assert_eq!(field(&fields, "far"), camera.far.to_string());
        assert!(details(&state, Item::Instance(usize::MAX)).is_none());
        assert!(details(&state, Item::Light(usize::MAX)).is_none());

        let mut scene = state.scene().clone();
        scene.instances[1].geometry = usize::MAX;
        scene.instances[1].material = usize::MAX;
        assert!(state.set_scene(scene));
        let (_, fields) = details(&state, Item::Instance(1)).unwrap();
        assert_eq!(field(&fields, "material"), format!("Index {}", usize::MAX));
        assert_eq!(field(&fields, "albedo"), "Unavailable");
    }
}
