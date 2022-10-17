/* @jsxImportSource solid-js */

const send = (url, key, value) => {
    fetch(url, {
        method: 'POST',
        body: `${key},${value}`
    })
}

export default function (props) {
    return (
        <>
            <button
                onClick={() => send(props.url, props.key, 1)}
                class="px-6 py-2.5 bg-blue-600 text-white rounded shadow hover:bg-blue-700 hover:shadow-lg active:bg-blue-800 active:shadow-lg transition duration-300">
                {props.children}
            </button>
        </>
    )
}